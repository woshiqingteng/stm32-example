/**
 * @file    i2c.c
 * @brief   Shared IIC master bus of the ALIENTEK F429 board (SCL = PH4,
 *          SDA = PH5).
 *
 * Two backends share one transaction API:
 *   - SW : software bit-bang master (default), half-period derived from speed_hz.
 *   - HW : hardware I2C2 peripheral (PH4/PH5, AF4). The peripheral is configured
 *          through HAL_I2C_Init; the transfers are driven with the HAL register
 *          flag macros (no HAL_I2C_Master_Transmit/Receive wrappers) in poll,
 *          interrupt or DMA transport.
 * All hardware facts live in the static i2c_hw_t descriptor (GPIO + DMA streams).
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "delay.h"
#include "gpio_hw.h"
#include "dma_hw.h"

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
    uint32_t     rcc_en;        /*!< RCC_APB1ENR_I2C2EN */
    IRQn_Type    irqn;          /*!< I2C2_EV_IRQn */
    uint32_t     af;            /*!< GPIO_AF4_I2C2 */
    gpio_hw_t    scl;           /*!< PH4 */
    gpio_hw_t    sda;           /*!< PH5 */
    dma_hw_t     dma[2];        /*!< [0] TX, [1] RX (8-bit, normal mode) */
} i2c_hw_t;

static const i2c_hw_t g_i2c_hw =
{
    .instance = I2C2,
    .rcc_en   = RCC_APB1ENR_I2C2EN,
    .irqn     = I2C2_EV_IRQn,
    .af       = GPIO_AF4_I2C2,
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

#define I2C_HW_TIMEOUT_COUNT    0x0000FFFFU  /* flag wait ~ a few ms */
#define I2C_XFER_TIMEOUT_COUNT  0x001FFFFFU  /* whole IT/DMA transfer */

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
static uint8_t           g_i2c_addr_phase;   /* HW: 1 = next byte is the address */
static volatile bool     g_i2c_dma_done[2];

/* ===================================================================== */
/*                       byte-level engine (POLL)                        */
/* ===================================================================== */

/* ---- software bit-bang ---- */

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

static uint8_t i2c_sw_read(uint8_t ack)
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

/* ---- hardware I2C2 (HAL flag macros, no transfer wrappers) ---- */

static uint8_t i2c_hw_wait_any(uint32_t f1, uint32_t f2)
{
    uint32_t time = I2C_HW_TIMEOUT_COUNT;

    while ((__HAL_I2C_GET_FLAG(&g_i2c_handle, f1) == RESET) &&
           ((f2 == 0U) || (__HAL_I2C_GET_FLAG(&g_i2c_handle, f2) == RESET)))
    {
        if (time-- == 0U)
        {


            return 1U;
        }
    }
    return 0U;
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

static void i2c_hw_start(void)
{
    g_i2c_hw.instance->CR1 &= ~I2C_CR1_STOP;   /* clear any stray STOP before START */
    g_i2c_hw.instance->CR1 |= I2C_CR1_START;
    (void)i2c_hw_wait_any(I2C_FLAG_SB, 0U);


    g_i2c_hw.instance->CR1 |= I2C_CR1_ACK;
    g_i2c_addr_phase = 1U;
}

static void i2c_hw_stop(void)
{
    uint32_t time = I2C_HW_TIMEOUT_COUNT;

    g_i2c_hw.instance->CR1 &= ~I2C_CR1_ACK;
    g_i2c_hw.instance->CR1 |= I2C_CR1_STOP;
    __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_FLAG_AF);
    while (((g_i2c_hw.instance->SR2 & I2C_SR2_BUSY) != 0U) && (time-- != 0U))
    {
    }
}

static void i2c_hw_send(uint8_t data)
{
    if (g_i2c_addr_phase == 0U)
    {
        (void)i2c_hw_wait_any(I2C_FLAG_TXE, 0U);
    }
    g_i2c_hw.instance->DR = data;

}

static uint8_t i2c_hw_wait_ack(void)
{
    if (g_i2c_addr_phase != 0U)
    {
        (void)i2c_hw_wait_any(I2C_FLAG_ADDR, I2C_FLAG_AF);
        g_i2c_addr_phase = 0U;
        if ((g_i2c_hw.instance->SR1 & I2C_SR1_AF) != 0U)
        {
            __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_FLAG_AF);
            return 1U;
        }
        (void)g_i2c_hw.instance->SR1;
        (void)g_i2c_hw.instance->SR2;   /* clear ADDR */
        return 0U;
    }

    (void)i2c_hw_wait_any(I2C_FLAG_BTF, I2C_FLAG_AF);
    if ((g_i2c_hw.instance->SR1 & I2C_SR1_AF) != 0U)
    {
        __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_FLAG_AF);
        return 1U;
    }
    return 0U;
}

static uint8_t i2c_hw_read(uint8_t ack)
{
    uint8_t data;

    if (ack != 0U)
    {
        g_i2c_hw.instance->CR1 |= I2C_CR1_ACK;
        (void)i2c_hw_wait_any(I2C_FLAG_RXNE, 0U);
        data = (uint8_t)g_i2c_hw.instance->DR;
    }
    else
    {
        g_i2c_hw.instance->CR1 &= ~I2C_CR1_ACK;
        (void)i2c_hw_wait_any(I2C_FLAG_RXNE, 0U);
        g_i2c_hw.instance->CR1 |= I2C_CR1_STOP;
        data = (uint8_t)g_i2c_hw.instance->DR;
    }
    return data;
}

/* ---- dispatch ---- */

static void i2c_ll_start(void)  { if (g_i2c.backend == I2C_BACKEND_HW) { i2c_hw_start(); }  else { i2c_sw_start(); } }
static void i2c_ll_stop(void)   { if (g_i2c.backend == I2C_BACKEND_HW) { i2c_hw_stop(); }   else { i2c_sw_stop(); } }
static void i2c_ll_send(uint8_t d){ if (g_i2c.backend == I2C_BACKEND_HW) { i2c_hw_send(d); } else { i2c_sw_send(d); } }
static uint8_t i2c_ll_read(uint8_t a){ return (g_i2c.backend == I2C_BACKEND_HW) ? i2c_hw_read(a) : i2c_sw_read(a); }
static uint8_t i2c_ll_wait_ack(void){ return (g_i2c.backend == I2C_BACKEND_HW) ? i2c_hw_wait_ack() : i2c_sw_wait_ack(); }

/* ===================================================================== */
/*                       POLL transaction layer                          */
/* ===================================================================== */

static bool i2c_poll_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    i2c_ll_start();
    i2c_ll_send((uint8_t)((addr7 << 1) | 0U));
    if (i2c_ll_wait_ack() != 0U)
    {
        i2c_ll_stop();
        return false;
    }
    for (i = 0U; i < len; i++)
    {
        i2c_ll_send(buf[i]);
        if (i2c_ll_wait_ack() != 0U)
        {
            i2c_ll_stop();
            return false;
        }
    }
    i2c_ll_stop();
    return true;
}

static bool i2c_poll_read(uint8_t addr7, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    i2c_ll_start();
    i2c_ll_send((uint8_t)((addr7 << 1) | 1U));
    if (i2c_ll_wait_ack() != 0U)
    {
        i2c_ll_stop();
        return false;
    }
    for (i = 0U; i < len; i++)
    {
        buf[i] = i2c_ll_read((i == (uint16_t)(len - 1U)) ? 0U : 1U);
    }
    i2c_ll_stop();
    return true;
}

static bool i2c_poll_write_read(uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                                uint8_t *rbuf, uint16_t rlen)
{
    uint16_t i;

    i2c_ll_start();
    i2c_ll_send((uint8_t)((addr7 << 1) | 0U));
    if (i2c_ll_wait_ack() != 0U)
    {
        i2c_ll_stop();
        return false;
    }
    for (i = 0U; i < wlen; i++)
    {
        i2c_ll_send(wbuf[i]);
        if (i2c_ll_wait_ack() != 0U)
        {
            i2c_ll_stop();
            return false;
        }
    }

    i2c_ll_start();   /* repeated start */
    i2c_ll_send((uint8_t)((addr7 << 1) | 1U));
    if (i2c_ll_wait_ack() != 0U)
    {
        i2c_ll_stop();
        return false;
    }
    for (i = 0U; i < rlen; i++)
    {
        rbuf[i] = i2c_ll_read((i == (uint16_t)(rlen - 1U)) ? 0U : 1U);
    }
    i2c_ll_stop();
    return true;
}

/* ===================================================================== */
/*                       interrupt transaction layer                     */
/* ===================================================================== */

static struct
{
    const uint8_t *wbuf;
    uint8_t       *rbuf;
    uint16_t       wlen, widx, rlen, ridx;
    uint8_t        addr7;
    uint8_t        phase;      /* 0 = write phase, 1 = read phase */
    bool           has_read;
    volatile bool  done;
    volatile bool  error;
} g_it;

static bool i2c_it_wait(void)
{
    uint32_t time = I2C_XFER_TIMEOUT_COUNT;

    while (!g_it.done && (time-- != 0U))
    {
    }
    g_i2c_hw.instance->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
    return (g_it.done && !g_it.error);
}

static bool i2c_it_xfer(uint8_t addr7,
                        const uint8_t *wbuf, uint16_t wlen,
                        uint8_t *rbuf, uint16_t rlen)
{
    g_it.wbuf  = wbuf;
    g_it.wlen  = wlen;
    g_it.widx  = 0U;
    g_it.rbuf  = rbuf;
    g_it.rlen  = rlen;
    g_it.ridx  = 0U;
    g_it.addr7 = addr7;
    g_it.has_read = (rlen != 0U);
    g_it.phase = (wlen != 0U) ? 0U : 1U;   /* read-only starts with the read address */
    g_it.done  = false;
    g_it.error = false;

    g_i2c_addr_phase = 0U;
    g_i2c_hw.instance->CR1 |= I2C_CR1_ACK;
    g_i2c_hw.instance->CR2 |= (I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
    g_i2c_hw.instance->CR1 |= I2C_CR1_START;

    return i2c_it_wait();
}

void I2C2_EV_IRQHandler(void)
{
    uint32_t sr1 = g_i2c_hw.instance->SR1;

    if ((sr1 & I2C_SR1_SB) != 0U)
    {
        g_i2c_hw.instance->DR = (uint8_t)((g_it.addr7 << 1) | ((g_it.phase != 0U) ? 1U : 0U));
    }
    else if ((sr1 & I2C_SR1_ADDR) != 0U)
    {
        (void)g_i2c_hw.instance->SR1;
        (void)g_i2c_hw.instance->SR2;   /* clear ADDR */
        if (g_it.phase != 0U)
        {
            g_i2c_hw.instance->CR1 |= I2C_CR1_ACK;
        }
    }

    if ((sr1 & I2C_SR1_TXE) != 0U)
    {
        if (g_it.widx < g_it.wlen)
        {
            g_i2c_hw.instance->DR = g_it.wbuf[g_it.widx++];
        }
        else
        {
            g_i2c_hw.instance->CR2 &= ~I2C_CR2_ITBUFEN;   /* wait for BTF */
        }
        sr1 = g_i2c_hw.instance->SR1;   /* BTF may have cleared on the DR write */
    }

    if (((sr1 & I2C_SR1_BTF) != 0U) && (g_it.phase == 0U) && (g_it.widx >= g_it.wlen))
    {
        if (g_it.has_read)
        {
            g_it.phase = 1U;
            g_i2c_hw.instance->CR1 |= I2C_CR1_START;   /* repeated start */
        }
        else
        {
            g_i2c_hw.instance->CR1 |= I2C_CR1_STOP;
            g_i2c_hw.instance->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);
            g_it.done = true;
        }
    }

    if ((sr1 & I2C_SR1_RXNE) != 0U)
    {
        if (g_it.ridx < (uint16_t)(g_it.rlen - 1U))
        {
            g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
        }
        else
        {
            g_i2c_hw.instance->CR1 &= ~I2C_CR1_ACK;
            g_i2c_hw.instance->CR1 |= I2C_CR1_STOP;
            g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
            g_i2c_hw.instance->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);
            g_it.done = true;
        }
    }
}

void I2C2_ER_IRQHandler(void)
{
    uint32_t err = (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR);

    if ((g_i2c_hw.instance->SR1 & err) != 0U)
    {
        g_i2c_hw.instance->SR1 = (uint16_t)~err;
        g_i2c_hw.instance->CR1 |= I2C_CR1_STOP;
        g_i2c_hw.instance->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
        g_it.error = true;
        g_it.done  = true;
    }
}

/* ===================================================================== */
/*                          DMA transaction layer                        */
/* ===================================================================== */

static void i2c_dma_irq(uint8_t dir)
{
    DMA_HandleTypeDef *hdma = &g_i2c_dma[dir];

    if (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma)) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
        g_i2c_dma_done[dir] = true;
    }
    if ((__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TE_FLAG_INDEX(hdma)) != RESET) ||
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_FE_FLAG_INDEX(hdma)) != RESET) ||
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_DME_FLAG_INDEX(hdma)) != RESET))
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TE_FLAG_INDEX(hdma));
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_FE_FLAG_INDEX(hdma));
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_DME_FLAG_INDEX(hdma));
        g_i2c_dma_done[dir] = true;   /* caller re-checks the data path */
    }
}

void DMA1_Stream7_IRQHandler(void)
{
    i2c_dma_irq(I2C_DMA_TX);
}

void DMA1_Stream2_IRQHandler(void)
{
    i2c_dma_irq(I2C_DMA_RX);
}

static bool i2c_dma_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    uint32_t time;


    i2c_hw_start();
    i2c_hw_send((uint8_t)((addr7 << 1) | 0U));
    if (i2c_hw_wait_ack() != 0U)
    {
        i2c_hw_stop();
        return false;
    }


    g_i2c_dma_done[I2C_DMA_TX] = false;
    g_i2c_hw.instance->CR2 |= I2C_CR2_DMAEN;
    __HAL_DMA_ENABLE_IT(&g_i2c_dma[I2C_DMA_TX], DMA_IT_TC | DMA_IT_TE);
    if (HAL_DMA_Start(&g_i2c_dma[I2C_DMA_TX], (uint32_t)buf,
                      (uint32_t)&g_i2c_hw.instance->DR, len) != HAL_OK)
    {
        g_i2c_hw.instance->CR2 &= ~I2C_CR2_DMAEN;
        i2c_hw_stop();
        return false;
    }


    time = I2C_XFER_TIMEOUT_COUNT;
    while (!g_i2c_dma_done[I2C_DMA_TX] && (time-- != 0U))
    {
    }

    (void)i2c_hw_wait_any(I2C_FLAG_BTF, 0U);   /* last byte shifted out */
    g_i2c_hw.instance->CR2 &= ~I2C_CR2_DMAEN;
    __HAL_DMA_DISABLE_IT(&g_i2c_dma[I2C_DMA_TX], DMA_IT_TC | DMA_IT_TE);
    i2c_hw_stop();

    return g_i2c_dma_done[I2C_DMA_TX];
}

static bool i2c_dma_read(uint8_t addr7, uint8_t *buf, uint16_t len)
{
    uint32_t time;

    if (len == 0U)
    {
        return true;
    }

    i2c_hw_start();
    i2c_hw_send((uint8_t)((addr7 << 1) | 1U));
    if (i2c_hw_wait_ack() != 0U)
    {
        i2c_hw_stop();
        return false;
    }

    g_i2c_dma_done[I2C_DMA_RX] = false;
    g_i2c_hw.instance->CR1 |= I2C_CR1_ACK;
    g_i2c_hw.instance->CR2 |= I2C_CR2_DMAEN;
    __HAL_DMA_ENABLE_IT(&g_i2c_dma[I2C_DMA_RX], DMA_IT_TC | DMA_IT_TE);
    if (HAL_DMA_Start(&g_i2c_dma[I2C_DMA_RX], (uint32_t)&g_i2c_hw.instance->DR,
                      (uint32_t)buf, len) != HAL_OK)
    {
        g_i2c_hw.instance->CR2 &= ~I2C_CR2_DMAEN;
        i2c_hw_stop();
        return false;
    }

    /* NACK and STOP just before the final byte arrives. */
    time = I2C_XFER_TIMEOUT_COUNT;
    while ((__HAL_DMA_GET_COUNTER(&g_i2c_dma[I2C_DMA_RX]) > 1U) && (time-- != 0U))
    {
    }
    g_i2c_hw.instance->CR1 &= ~I2C_CR1_ACK;
    g_i2c_hw.instance->CR1 |= I2C_CR1_STOP;

    time = I2C_XFER_TIMEOUT_COUNT;
    while (!g_i2c_dma_done[I2C_DMA_RX] && (time-- != 0U))
    {
    }
    g_i2c_hw.instance->CR2 &= ~I2C_CR2_DMAEN;
    __HAL_DMA_DISABLE_IT(&g_i2c_dma[I2C_DMA_RX], DMA_IT_TC | DMA_IT_TE);

    return g_i2c_dma_done[I2C_DMA_RX];
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

        g_i2c_addr_phase = 0U;
        g_i2c_hw.instance->CR1 |= I2C_CR1_STOP;   /* release the bus */



        if (g_i2c.xfer == I2C_XFER_DMA)
        {
            dma_hw_setup(&g_i2c_dma[I2C_DMA_TX], &g_i2c_hw.dma[I2C_DMA_TX]);
            dma_hw_setup(&g_i2c_dma[I2C_DMA_RX], &g_i2c_hw.dma[I2C_DMA_RX]);
            HAL_NVIC_SetPriority(g_i2c_hw.dma[I2C_DMA_TX].irqn, 3U, 3U);
            HAL_NVIC_EnableIRQ(g_i2c_hw.dma[I2C_DMA_TX].irqn);
            HAL_NVIC_SetPriority(g_i2c_hw.dma[I2C_DMA_RX].irqn, 3U, 3U);
            HAL_NVIC_EnableIRQ(g_i2c_hw.dma[I2C_DMA_RX].irqn);
        }
        else if (g_i2c.xfer == I2C_XFER_IT)
        {
            HAL_NVIC_SetPriority(g_i2c_hw.irqn, 3U, 3U);
            HAL_NVIC_EnableIRQ(g_i2c_hw.irqn);
            HAL_NVIC_SetPriority(I2C2_ER_IRQn, 3U, 3U);
            HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
        }
        else
        {
            /* polled hardware: no interrupt needed */
        }
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

    if ((len == 0U) || (g_i2c.backend == I2C_BACKEND_SW))
    {
        return i2c_poll_write(addr7, buf, len);   /* probe / bit-bang */
    }
    if (g_i2c.xfer == I2C_XFER_DMA)
    {
        return i2c_dma_write(addr7, buf, len);
    }
    if (g_i2c.xfer == I2C_XFER_IT)
    {
        return i2c_it_xfer(addr7, buf, len, 0, 0U);
    }
    return i2c_poll_write(addr7, buf, len);
}

bool i2c_read(i2c_device_t dev, uint8_t *buf, uint16_t len)
{
    uint8_t addr7;

    if (!g_i2c.ready || (dev >= I2C_DEV_NUM))
    {
        return false;
    }
    addr7 = g_i2c_dev_addr[dev];

    if (g_i2c.backend == I2C_BACKEND_SW)
    {
        return i2c_poll_read(addr7, buf, len);
    }
    if (g_i2c.xfer == I2C_XFER_DMA)
    {
        return i2c_dma_read(addr7, buf, len);
    }
    if (g_i2c.xfer == I2C_XFER_IT)
    {
        return i2c_it_xfer(addr7, 0, 0U, buf, len);
    }
    return i2c_poll_read(addr7, buf, len);
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
    addr7 = g_i2c_dev_addr[dev];

    /* write/read under the hardware backend uses IT, or the polled engine
     * when IT is not selected (DMA does not cover the repeated-start read). */
    if ((g_i2c.backend == I2C_BACKEND_HW) && (g_i2c.xfer == I2C_XFER_IT))
    {
        return i2c_it_xfer(addr7, wbuf, wlen, rbuf, rlen);
    }
    return i2c_poll_write_read(addr7, wbuf, wlen, rbuf, rlen);
}
