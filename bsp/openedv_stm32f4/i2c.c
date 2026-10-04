/**
 * @file    i2c.c
 * @brief   Shared IIC master bus of the ALIENTEK F429 board (SCL = PH4,
 *          SDA = PH5).
 *
 * Two backends share one transaction API:
 *   - SW : software bit-bang master (default), half-period derived from speed_hz.
 *   - HW : hardware I2C2 peripheral (PH4/PH5, AF4). The peripheral and every
 *          transfer are driven through the HAL (blocking / interrupt / DMA); the
 *          HAL IRQ dispatch and DMA completion are used as-is.
 * All hardware facts live in the static i2c_hw_t descriptor (GPIO + DMA streams).
 * Errors are not latched (usart style): a failed transfer just discards and
 * returns false.
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "delay.h"
#include "gpio_hw.h"
#include "dma_hw.h"

#define I2C_TIMEOUT_MS  100U  /* HAL blocking / transfer-completion budget */

/* ===== 7-bit slave addresses ===== */

static const uint8_t g_i2c_dev_addr[I2C_DEV_NUM] =
{
    [I2C_DEV_EEPROM]    = 0x50U,
    [I2C_DEV_CODEC]     = 0x10U,
    [I2C_DEV_ALS]       = 0x1EU,
    [I2C_DEV_IMU]       = 0x36U,
    [I2C_DEV_MAG]       = 0x0CU,
    [I2C_DEV_IO_EXPAND] = 0x20U,
};

/* ===== hardware descriptor ===== */

enum { I2C_DMA_TX = 0, I2C_DMA_RX = 1 };

typedef struct
{
    I2C_TypeDef *instance;      /*!< I2C2 */
    IRQn_Type    irqn;          /*!< I2C2_EV_IRQn */
    gpio_hw_t    scl;           /*!< PH4 */
    gpio_hw_t    sda;           /*!< PH5 */
    dma_hw_t     dma[2];        /*!< [0] TX, [1] RX (8-bit, normal mode) */
} i2c_hw_t;

static const i2c_hw_t g_i2c_hw =
{
    .instance = I2C2,
    .irqn     = I2C2_EV_IRQn,
    .scl      = { GPIOH, RCC_AHB1ENR_GPIOHEN, GPIO_PIN_4, GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF4_I2C2 },
    .sda      = { GPIOH, RCC_AHB1ENR_GPIOHEN, GPIO_PIN_5, GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF4_I2C2 },
    .dma      = {
        {   /* TX: memory -> I2C2->DR */
            .rcc_en         = RCC_AHB1ENR_DMA1EN,
            .stream         = DMA1_Stream7,
            .irqn           = DMA1_Stream7_IRQn,
            .channel        = DMA_CHANNEL_7,
            .direction      = DMA_MEMORY_TO_PERIPH,
            .periph_inc     = DMA_PINC_DISABLE,
            .mem_inc        = DMA_MINC_ENABLE,
            .periph_align   = DMA_PDATAALIGN_BYTE,
            .mem_align      = DMA_MDATAALIGN_BYTE,
            .mode           = DMA_NORMAL,
            .priority       = DMA_PRIORITY_MEDIUM,
            .fifo_mode      = DMA_FIFOMODE_DISABLE,
            .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst      = DMA_MBURST_SINGLE,
            .periph_burst   = DMA_PBURST_SINGLE,
        },
        {   /* RX: I2C2->DR -> memory */
            .rcc_en         = RCC_AHB1ENR_DMA1EN,
            .stream         = DMA1_Stream2,
            .irqn           = DMA1_Stream2_IRQn,
            .channel        = DMA_CHANNEL_7,
            .direction      = DMA_PERIPH_TO_MEMORY,
            .periph_inc     = DMA_PINC_DISABLE,
            .mem_inc        = DMA_MINC_ENABLE,
            .periph_align   = DMA_PDATAALIGN_BYTE,
            .mem_align      = DMA_MDATAALIGN_BYTE,
            .mode           = DMA_NORMAL,
            .priority       = DMA_PRIORITY_MEDIUM,
            .fifo_mode      = DMA_FIFOMODE_DISABLE,
            .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst      = DMA_MBURST_SINGLE,
            .periph_burst   = DMA_PBURST_SINGLE,
        },
    },
};

/* ===== per-bus state ===== */

static struct
{
    i2c_backend_t backend;
    i2c_xfer_t    xfer;
    uint32_t      speed_hz;
    uint32_t      delay_us;
    bool          ready;
} g_i2c;

static I2C_HandleTypeDef g_i2c_handle;
static DMA_HandleTypeDef g_i2c_dma[2];

/* ===================================================================== */
/*                       software bit-bang backend                       */
/* ===================================================================== */

#define SW_SCL(x)   HAL_GPIO_WritePin(g_i2c_hw.scl.port, g_i2c_hw.scl.pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SW_SDA(x)   HAL_GPIO_WritePin(g_i2c_hw.sda.port, g_i2c_hw.sda.pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SW_SDA_RD   HAL_GPIO_ReadPin(g_i2c_hw.sda.port, g_i2c_hw.sda.pin)

static void i2c_sw_start(void)
{
    SW_SDA(1);
    SW_SCL(1);
    delay_us(g_i2c.delay_us);
    SW_SDA(0);
    delay_us(g_i2c.delay_us);
    SW_SCL(0);
    delay_us(g_i2c.delay_us);
}

static void i2c_sw_stop(void)
{
    SW_SDA(0);
    delay_us(g_i2c.delay_us);
    SW_SCL(1);
    delay_us(g_i2c.delay_us);
    SW_SDA(1);
    delay_us(g_i2c.delay_us);
}

static void i2c_sw_send(uint8_t data)
{
    uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
        SW_SDA((data & 0x80U) >> 7);
        delay_us(g_i2c.delay_us);
        SW_SCL(1);
        delay_us(g_i2c.delay_us);
        SW_SCL(0);
        data <<= 1;
    }
    SW_SDA(1);
}

static uint8_t i2c_sw_wait_ack(void)
{
    uint8_t waittime = 0U;

    SW_SDA(1);
    delay_us(g_i2c.delay_us);
    SW_SCL(1);
    delay_us(g_i2c.delay_us);

    while (SW_SDA_RD != 0U)
    {
        if (++waittime > 250U)
        {
            i2c_sw_stop();
            return 1U;
        }
        delay_us(g_i2c.delay_us);
    }
    SW_SCL(0);
    delay_us(g_i2c.delay_us);
    return 0U;
}

static uint8_t i2c_sw_read_byte(uint8_t ack)
{
    uint8_t i;
    uint8_t data = 0U;

    for (i = 0U; i < 8U; i++)
    {
        data <<= 1;
        SW_SCL(1);
        delay_us(g_i2c.delay_us);
        if (SW_SDA_RD != 0U)
        {
            data++;
        }
        SW_SCL(0);
        delay_us(g_i2c.delay_us);
    }

    SW_SDA(ack == 0U ? 1 : 0);
    delay_us(g_i2c.delay_us);
    SW_SCL(1);
    delay_us(g_i2c.delay_us);
    SW_SCL(0);
    delay_us(g_i2c.delay_us);
    SW_SDA(1);
    delay_us(g_i2c.delay_us);
    return data;
}

static bool i2c_sw_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    i2c_sw_start();
    i2c_sw_send((uint8_t)((addr7 << 1) | 0U));
    if (i2c_sw_wait_ack() != 0U)
    {
        i2c_sw_stop();
        return false;
    }
    for (i = 0U; i < len; i++)
    {
        i2c_sw_send(buf[i]);
        if (i2c_sw_wait_ack() != 0U)
        {
            i2c_sw_stop();
            return false;
        }
    }
    i2c_sw_stop();
    return true;
}

static bool i2c_sw_read(uint8_t addr7, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    i2c_sw_start();
    i2c_sw_send((uint8_t)((addr7 << 1) | 1U));
    if (i2c_sw_wait_ack() != 0U)
    {
        i2c_sw_stop();
        return false;
    }
    for (i = 0U; i < len; i++)
    {
        buf[i] = i2c_sw_read_byte((i == (uint16_t)(len - 1U)) ? 0U : 1U);
    }
    i2c_sw_stop();
    return true;
}

static bool i2c_sw_write_read(uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                              uint8_t *rbuf, uint16_t rlen)
{
    uint16_t i;

    i2c_sw_start();
    i2c_sw_send((uint8_t)((addr7 << 1) | 0U));
    if (i2c_sw_wait_ack() != 0U)
    {
        i2c_sw_stop();
        return false;
    }
    for (i = 0U; i < wlen; i++)
    {
        i2c_sw_send(wbuf[i]);
        if (i2c_sw_wait_ack() != 0U)
        {
            i2c_sw_stop();
            return false;
        }
    }

    i2c_sw_start();   /* repeated start */
    i2c_sw_send((uint8_t)((addr7 << 1) | 1U));
    if (i2c_sw_wait_ack() != 0U)
    {
        i2c_sw_stop();
        return false;
    }
    for (i = 0U; i < rlen; i++)
    {
        rbuf[i] = i2c_sw_read_byte((i == (uint16_t)(rlen - 1U)) ? 0U : 1U);
    }
    i2c_sw_stop();
    return true;
}

/* ===================================================================== */
/*                       hardware I2C2 backend (HAL)                     */
/* ===================================================================== */

/* Wait for an IT/DMA transfer to finish. On timeout the transfer is discarded
 * (DMA aborted, I2C interrupts masked); no error is latched. */
static bool i2c_hw_wait(void)
{
    uint32_t start = HAL_GetTick();

    while (HAL_I2C_GetState(&g_i2c_handle) != HAL_I2C_STATE_READY)
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_TX]);
            (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
            __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF);
            return false;
        }
    }
    return (HAL_I2C_GetError(&g_i2c_handle) == HAL_I2C_ERROR_NONE);
}

static bool i2c_hw_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    uint16_t dev = (uint16_t)(addr7 << 1);

    if (len == 0U)   /* probe */
    {
        return (HAL_I2C_IsDeviceReady(&g_i2c_handle, dev, 1U, I2C_TIMEOUT_MS) == HAL_OK);
    }

    if (g_i2c.xfer == I2C_XFER_IT)
    {
        return (HAL_I2C_Master_Transmit_IT(&g_i2c_handle, dev, (uint8_t *)buf, len) == HAL_OK) &&
               i2c_hw_wait();
    }
    if (g_i2c.xfer == I2C_XFER_DMA)
    {
        return (HAL_I2C_Master_Transmit_DMA(&g_i2c_handle, dev, (uint8_t *)buf, len) == HAL_OK) &&
               i2c_hw_wait();
    }
    return (HAL_I2C_Master_Transmit(&g_i2c_handle, dev, (uint8_t *)buf, len, I2C_TIMEOUT_MS) == HAL_OK);
}

static bool i2c_hw_read(uint8_t addr7, uint8_t *buf, uint16_t len)
{
    uint16_t dev = (uint16_t)(addr7 << 1);

    if (g_i2c.xfer == I2C_XFER_IT)
    {
        return (HAL_I2C_Master_Receive_IT(&g_i2c_handle, dev, buf, len) == HAL_OK) &&
               i2c_hw_wait();
    }
    if (g_i2c.xfer == I2C_XFER_DMA)
    {
        return (HAL_I2C_Master_Receive_DMA(&g_i2c_handle, dev, buf, len) == HAL_OK) &&
               i2c_hw_wait();
    }
    return (HAL_I2C_Master_Receive(&g_i2c_handle, dev, buf, len, I2C_TIMEOUT_MS) == HAL_OK);
}

static bool i2c_hw_write_read(uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                              uint8_t *rbuf, uint16_t rlen)
{
    uint16_t dev = (uint16_t)(addr7 << 1);
    uint16_t reg;
    uint16_t reg_size;

    if (wlen == 1U)
    {
        reg      = wbuf[0];
        reg_size = I2C_MEMADD_SIZE_8BIT;
    }
    else if (wlen == 2U)
    {
        reg      = (uint16_t)(((uint16_t)wbuf[0] << 8) | wbuf[1]);
        reg_size = I2C_MEMADD_SIZE_16BIT;
    }
    else
    {
        return false;   /* only 1/2-byte register prefixes are used on this bus */
    }

    if (g_i2c.xfer == I2C_XFER_IT)
    {
        return (HAL_I2C_Mem_Read_IT(&g_i2c_handle, dev, reg, reg_size, rbuf, rlen) == HAL_OK) &&
               i2c_hw_wait();
    }
    if (g_i2c.xfer == I2C_XFER_DMA)
    {
        return (HAL_I2C_Mem_Read_DMA(&g_i2c_handle, dev, reg, reg_size, rbuf, rlen) == HAL_OK) &&
               i2c_hw_wait();
    }
    return (HAL_I2C_Mem_Read(&g_i2c_handle, dev, reg, reg_size, rbuf, rlen, I2C_TIMEOUT_MS) == HAL_OK);
}

/* ---- interrupts (HAL dispatch) ---- */

void I2C2_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&g_i2c_handle);
}

void I2C2_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&g_i2c_handle);
}

void DMA1_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_i2c_dma[I2C_DMA_TX]);
}

void DMA1_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_i2c_dma[I2C_DMA_RX]);
}

/* Clock SCL to release a slave that holds SDA after an aborted transfer. */
static void i2c_hw_bus_recover(void)
{
    gpio_hw_t scl = g_i2c_hw.scl;
    gpio_hw_t sda = g_i2c_hw.sda;
    uint8_t i;

    scl.mode      = GPIO_MODE_OUTPUT_OD;
    scl.alternate = 0U;
    sda.mode      = GPIO_MODE_OUTPUT_OD;
    sda.alternate = 0U;
    gpio_hw_setup(&scl);
    gpio_hw_setup(&sda);

    HAL_GPIO_WritePin(sda.port, sda.pin, GPIO_PIN_SET);
    for (i = 0U; i < 9U; i++)
    {
        HAL_GPIO_WritePin(scl.port, scl.pin, GPIO_PIN_RESET);
        delay_us(5U);
        HAL_GPIO_WritePin(scl.port, scl.pin, GPIO_PIN_SET);
        delay_us(5U);
    }
    HAL_GPIO_WritePin(scl.port, scl.pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sda.port, sda.pin, GPIO_PIN_RESET);
    delay_us(5U);
    HAL_GPIO_WritePin(scl.port, scl.pin, GPIO_PIN_SET);
    delay_us(5U);
    HAL_GPIO_WritePin(sda.port, sda.pin, GPIO_PIN_SET);
    delay_us(5U);
}

/* ===================================================================== */
/*                       initialisation                                  */
/* ===================================================================== */

void i2c_init(const i2c_cfg_t *cfg)
{
    static const i2c_cfg_t cfg_default = { I2C_CFG_DEFAULT };
    const i2c_cfg_t *c = cfg;

    if (c == 0)
    {
        if (g_i2c.ready)
        {
            return;
        }
        c = &cfg_default;
    }

    g_i2c.backend  = c->backend;
    g_i2c.xfer     = c->xfer;
    g_i2c.speed_hz = (c->speed_hz != 0U) ? c->speed_hz : 100000U;
    g_i2c.delay_us = 500000U / g_i2c.speed_hz;
    if (g_i2c.delay_us < 2U)
    {
        g_i2c.delay_us = 2U;
    }

    if (g_i2c.backend == I2C_BACKEND_HW)
    {
        __HAL_RCC_I2C2_CLK_ENABLE();
        i2c_hw_bus_recover();
        gpio_hw_setup(&g_i2c_hw.scl);
        gpio_hw_setup(&g_i2c_hw.sda);

        g_i2c_handle.Instance             = g_i2c_hw.instance;
        g_i2c_handle.Init.ClockSpeed      = g_i2c.speed_hz;
        g_i2c_handle.Init.DutyCycle       = I2C_DUTYCYCLE_2;
        g_i2c_handle.Init.OwnAddress1     = 0U;
        g_i2c_handle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
        g_i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        g_i2c_handle.Init.OwnAddress2     = 0U;
        g_i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        g_i2c_handle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
        (void)HAL_I2C_Init(&g_i2c_handle);

        /* DMA streams (DMA transport). */
        dma_hw_setup(&g_i2c_dma[I2C_DMA_TX], &g_i2c_hw.dma[I2C_DMA_TX]);
        dma_hw_setup(&g_i2c_dma[I2C_DMA_RX], &g_i2c_hw.dma[I2C_DMA_RX]);
        __HAL_LINKDMA(&g_i2c_handle, hdmatx, g_i2c_dma[I2C_DMA_TX]);
        __HAL_LINKDMA(&g_i2c_handle, hdmarx, g_i2c_dma[I2C_DMA_RX]);

        /* NVIC: I2C events/errors (IT + DMA) and the two DMA streams. */
        HAL_NVIC_SetPriority(g_i2c_hw.irqn, 3U, 3U);
        HAL_NVIC_EnableIRQ(g_i2c_hw.irqn);
        HAL_NVIC_SetPriority(I2C2_ER_IRQn, 3U, 3U);
        HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
        HAL_NVIC_SetPriority(g_i2c_hw.dma[I2C_DMA_TX].irqn, 3U, 3U);
        HAL_NVIC_EnableIRQ(g_i2c_hw.dma[I2C_DMA_TX].irqn);
        HAL_NVIC_SetPriority(g_i2c_hw.dma[I2C_DMA_RX].irqn, 3U, 3U);
        HAL_NVIC_EnableIRQ(g_i2c_hw.dma[I2C_DMA_RX].irqn);
    }
    else
    {
        gpio_hw_t scl = g_i2c_hw.scl;
        gpio_hw_t sda = g_i2c_hw.sda;

        scl.mode      = GPIO_MODE_OUTPUT_PP;
        scl.alternate = 0U;
        sda.mode      = GPIO_MODE_OUTPUT_OD;
        sda.alternate = 0U;
        gpio_hw_setup(&scl);
        gpio_hw_setup(&sda);
        i2c_sw_stop();
    }

    g_i2c.ready = true;
}

/* ===================================================================== */
/*                       public API                                      */
/* ===================================================================== */

bool i2c_write(i2c_device_t dev, const uint8_t *buf, uint16_t len)
{
    uint8_t addr7;

    if (!g_i2c.ready || (dev >= I2C_DEV_NUM))
    {
        return false;
    }
    addr7 = g_i2c_dev_addr[dev];

    if (g_i2c.backend == I2C_BACKEND_SW)
    {
        return i2c_sw_write(addr7, buf, len);
    }
    return i2c_hw_write(addr7, buf, len);
}

bool i2c_read(i2c_device_t dev, uint8_t *buf, uint16_t len)
{
    uint8_t addr7;

    if (!g_i2c.ready || (dev >= I2C_DEV_NUM))
    {
        return false;
    }
    if (len == 0U)
    {
        return true;   /* nothing to receive */
    }
    addr7 = g_i2c_dev_addr[dev];

    if (g_i2c.backend == I2C_BACKEND_SW)
    {
        return i2c_sw_read(addr7, buf, len);
    }
    return i2c_hw_read(addr7, buf, len);
}

bool i2c_write_read(i2c_device_t dev,
                    const uint8_t *wbuf, uint16_t wlen,
                    uint8_t *rbuf, uint16_t rlen)
{
    uint8_t addr7;

    if (!g_i2c.ready || (dev >= I2C_DEV_NUM))
    {
        return false;
    }
    if (rlen == 0U)
    {
        return i2c_write(dev, wbuf, wlen);   /* no read phase */
    }
    if (wlen == 0U)
    {
        return i2c_read(dev, rbuf, rlen);    /* no write prefix */
    }
    addr7 = g_i2c_dev_addr[dev];

    if (g_i2c.backend == I2C_BACKEND_SW)
    {
        return i2c_sw_write_read(addr7, wbuf, wlen, rbuf, rlen);
    }
    return i2c_hw_write_read(addr7, wbuf, wlen, rbuf, rlen);
}
