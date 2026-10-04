/**
 * @file    i2c.c
 * @brief   Shared IIC master bus of the ALIENTEK F429 board (SCL = PH4,
 *          SDA = PH5).
 *
 * One transaction API, one runtime transport selector (i2c_io_t):
 *   - I2C_IO_SW  : software bit-bang master (default), fixed 100 kHz.
 *   - I2C_IO_POLL/I2C_IO_IT/I2C_IO_DMA : hardware I2C2 peripheral (PH4/PH5,
 *     AF4).  Configured with HAL_I2C_Init; the poll / interrupt / DMA transfers
 *     are implemented here with the flag macros (mirroring the HAL I2C v1
 *     sequences), so the HAL transfer wrappers are not used.
 * All hardware facts live in the static i2c_hw_t descriptor (GPIO + DMA streams).
 * Errors are not latched (usart style): a failed transfer just discards and
 * returns false.
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "delay.h"
#include "gpio_hw.h"
#include "dma_hw.h"

#define I2C_TIMEOUT_MS  100U  /* flag wait / transfer-completion budget */

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
    i2c_io_t io;
    uint32_t speed_hz;
    bool     ready;
} g_i2c;

static I2C_HandleTypeDef g_i2c_handle;
static DMA_HandleTypeDef g_i2c_dma[2];
static volatile bool     g_dma_done[2];
static volatile bool     g_dma_err[2];

static uint8_t i2c_ll_dev(uint8_t addr7, uint8_t rw)
{
    return (uint8_t)((addr7 << 1) | (rw & 1U));
}

/* ===================================================================== */
/*                       software bit-bang backend                       */
/* ===================================================================== */

/* The software bit-bang runs at a fixed 100 kHz (5 us half-period). */
#define I2C_SW_HALF_US  5U

#define SW_SCL(x)   HAL_GPIO_WritePin(g_i2c_hw.scl.port, g_i2c_hw.scl.pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SW_SDA(x)   HAL_GPIO_WritePin(g_i2c_hw.sda.port, g_i2c_hw.sda.pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SW_SDA_RD   HAL_GPIO_ReadPin(g_i2c_hw.sda.port, g_i2c_hw.sda.pin)

static void i2c_sw_start(void)
{
    SW_SDA(1);
    SW_SCL(1);
    delay_us(I2C_SW_HALF_US);
    SW_SDA(0);
    delay_us(I2C_SW_HALF_US);
    SW_SCL(0);
    delay_us(I2C_SW_HALF_US);
}

static void i2c_sw_stop(void)
{
    SW_SDA(0);
    delay_us(I2C_SW_HALF_US);
    SW_SCL(1);
    delay_us(I2C_SW_HALF_US);
    SW_SDA(1);
    delay_us(I2C_SW_HALF_US);
}

static void i2c_sw_send(uint8_t data)
{
    uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
        SW_SDA((data & 0x80U) >> 7);
        delay_us(I2C_SW_HALF_US);
        SW_SCL(1);
        delay_us(I2C_SW_HALF_US);
        SW_SCL(0);
        data <<= 1;
    }
    SW_SDA(1);
}

static uint8_t i2c_sw_wait_ack(void)
{
    uint8_t waittime = 0U;

    SW_SDA(1);
    delay_us(I2C_SW_HALF_US);
    SW_SCL(1);
    delay_us(I2C_SW_HALF_US);

    while (SW_SDA_RD != 0U)
    {
        if (++waittime > 250U)
        {
            i2c_sw_stop();
            return 1U;
        }
        delay_us(I2C_SW_HALF_US);
    }
    SW_SCL(0);
    delay_us(I2C_SW_HALF_US);
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
        delay_us(I2C_SW_HALF_US);
        if (SW_SDA_RD != 0U)
        {
            data++;
        }
        SW_SCL(0);
        delay_us(I2C_SW_HALF_US);
    }

    SW_SDA(ack == 0U ? 1 : 0);
    delay_us(I2C_SW_HALF_US);
    SW_SCL(1);
    delay_us(I2C_SW_HALF_US);
    SW_SCL(0);
    delay_us(I2C_SW_HALF_US);
    SW_SDA(1);
    delay_us(I2C_SW_HALF_US);
    return data;
}

static bool i2c_sw_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    i2c_sw_start();
    i2c_sw_send(i2c_ll_dev(addr7, 0U));
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
    i2c_sw_send(i2c_ll_dev(addr7, 1U));
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
    i2c_sw_send(i2c_ll_dev(addr7, 0U));
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
    i2c_sw_send(i2c_ll_dev(addr7, 1U));
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
/*                       hardware I2C2 backend (flag macros)             */
/* ===================================================================== */

/* Master transfer context (shared by the poll / interrupt transports). */
enum { I2C_IT_TX = 0, I2C_IT_RX, I2C_IT_MEMR };

static struct
{
    uint8_t        kind;
    uint8_t        addr7;
    uint8_t        phase;     /* MEMR: 0 = prefix write, 1 = read */
    uint8_t        event;     /* MEMR prefix: HAL-style EventCount */
    const uint8_t *wbuf;
    uint16_t       wlen, widx;
    uint8_t       *rbuf;
    uint16_t       rlen, ridx;
    volatile bool  done;
    volatile bool  error;
} g_it;

#define I2C_IT_RETRY   3U   /* re-try a transaction that left the bus stuck */

/* Shared protocol engine (defined with the interrupt transport below). */
static void i2c_proto_step(uint32_t sr1, uint32_t sr2);

static void i2c_hw_bus_recover(void);   /* defined with the HW back-end below */

/* Recover a bus held low after a failed transfer, then re-init the peripheral
 * and re-enable it (used as the retry step by the poll and interrupt paths). */
static void i2c_hw_bus_reset(void)
{
    __HAL_I2C_DISABLE(&g_i2c_handle);
    i2c_hw_bus_recover();
    gpio_hw_setup(&g_i2c_hw.scl);
    gpio_hw_setup(&g_i2c_hw.sda);
    (void)HAL_I2C_Init(&g_i2c_handle);
    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_PE);
}

/* Wait until __FLAG__ == WANT (SET/RESET). false on timeout. */
static bool i2c_hw_wait(uint32_t flag, FlagStatus want)
{
    uint32_t start = HAL_GetTick();

    while (__HAL_I2C_GET_FLAG(&g_i2c_handle, flag) != want)
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            return false;
        }
    }
    return true;
}

/* Wait for the address phase (ADDR set, or NACK with AF). Leaves ADDR set. */
static bool i2c_hw_wait_addr(void)
{
    uint32_t start = HAL_GetTick();

    while ((__HAL_I2C_GET_FLAG(&g_i2c_handle, I2C_FLAG_ADDR) == RESET) &&
           (__HAL_I2C_GET_FLAG(&g_i2c_handle, I2C_FLAG_AF) == RESET))
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            return false;
        }
    }
    if (__HAL_I2C_GET_FLAG(&g_i2c_handle, I2C_FLAG_AF) == SET)
    {
        __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_FLAG_AF);
        return false;
    }
    return true;
}

static void i2c_hw_stop(void)
{
    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
    __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_FLAG_AF);
}

/* Probe: is the device present at @p addr7? */
static bool i2c_hw_probe(uint8_t addr7)
{
    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);
    if (!i2c_hw_wait(I2C_FLAG_SB, SET))
    {
        i2c_hw_stop();
        return false;
    }
    g_i2c_hw.instance->DR = i2c_ll_dev(addr7, 0U);
    if (!i2c_hw_wait_addr())
    {
        i2c_hw_stop();
        return false;
    }
    __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);
    i2c_hw_stop();
    return true;
}

/* Generic polled transfer: set the context up, start, then drive the shared
 * protocol engine from the main loop (exactly the events the IT ISR feeds), so
 * the poll / interrupt / DMA transports share one read tail and MEM prefix. */
static bool i2c_hw_poll(uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                        uint8_t *rbuf, uint16_t rlen)
{
    uint8_t attempt;

    g_it.addr7 = addr7;
    g_it.wbuf  = wbuf;
    g_it.wlen  = wlen;
    g_it.rbuf  = rbuf;
    g_it.rlen  = rlen;
    if (rlen == 0U)
    {
        g_it.kind = I2C_IT_TX;
    }
    else if (wlen == 0U)
    {
        g_it.kind = I2C_IT_RX;
    }
    else
    {
        g_it.kind = I2C_IT_MEMR;
    }

    for (attempt = 0U; attempt < I2C_IT_RETRY; attempt++)
    {
        uint32_t start;

        if (attempt == 0U)
        {
            /* A previous transfer may have left the peripheral mid-transfer. */
            if ((g_i2c_hw.instance->SR2 & (I2C_SR2_BUSY | I2C_SR2_MSL)) != 0U)
            {
                __HAL_I2C_DISABLE(&g_i2c_handle);
                __HAL_I2C_ENABLE(&g_i2c_handle);
            }
        }
        else
        {
            i2c_hw_bus_reset();
        }

        g_it.widx  = 0U;
        g_it.ridx  = 0U;
        g_it.event = 0U;
        g_it.done  = false;
        g_it.error = false;
        g_it.phase = (g_it.kind == I2C_IT_RX) ? 1U : 0U;

        if (!i2c_hw_wait(I2C_FLAG_BUSY, RESET))
        {
            continue;
        }
        CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_POS);
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);

        start = HAL_GetTick();
        while (!g_it.done)
        {
            uint32_t sr1;
            uint32_t sr2;

            if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
            {
                i2c_hw_stop();
                break;
            }
            sr1 = g_i2c_hw.instance->SR1;   /* read SR1 before SR2 (clears ADDR) */
            sr2 = g_i2c_hw.instance->SR2;
            i2c_proto_step(sr1, sr2);
        }
        if (g_it.done && !g_it.error)
        {
            return true;
        }
    }
    return false;
}

/* ===================================================================== */
/*                       hardware I2C2 interrupt transport               */
/* ===================================================================== */

static void i2c_hw_bus_recover(void);   /* defined with the HW back-end */

static bool i2c_is_read(void)
{
    return (g_it.kind == I2C_IT_RX) || ((g_it.kind == I2C_IT_MEMR) && (g_it.phase != 0U));
}

static void i2c_done(void)
{
    __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
    g_it.done = true;
}

/* Read tail for the IT receiver (mirrors I2C_MasterReceive_RXNE/BTF). */
static void i2c_proto_rxne(void)
{
    uint16_t n = (uint16_t)(g_it.rlen - g_it.ridx);

    if (n > 3U)
    {
        g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
        if ((uint16_t)(g_it.rlen - g_it.ridx) == 3U)
        {
            __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_BUF);
        }
    }
    else if (n == 1U)
    {
        CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
        g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
        i2c_done();
    }
    else
    {
        __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_BUF);
    }
}

static void i2c_proto_rx_btf(void)
{
    uint16_t n = (uint16_t)(g_it.rlen - g_it.ridx);

    if (n == 4U)
    {
        __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_BUF);
        g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
    }
    else if (n == 3U)
    {
        __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_BUF);
        CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
        g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
    }
    else if (n == 2U)
    {
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
        g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
        g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
        i2c_done();
    }
    else
    {
        g_it.rbuf[g_it.ridx++] = (uint8_t)g_i2c_hw.instance->DR;
    }
}

static bool i2c_it_wait(void)
{
    uint32_t start = HAL_GetTick();

    while (!g_it.done)
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
            __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
            return false;
        }
    }
    return !g_it.error;
}

static bool i2c_it_xfer(uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                        uint8_t *rbuf, uint16_t rlen)
{
    uint8_t attempt;

    g_it.addr7 = addr7;
    g_it.wbuf  = wbuf;
    g_it.wlen  = wlen;
    g_it.rbuf  = rbuf;
    g_it.rlen  = rlen;

    if (rlen == 0U)
    {
        g_it.kind  = I2C_IT_TX;
        g_it.phase = 0U;
    }
    else if (wlen == 0U)
    {
        g_it.kind  = I2C_IT_RX;
        g_it.phase = 1U;
    }
    else
    {
        g_it.kind  = I2C_IT_MEMR;
        g_it.phase = 0U;
    }

    for (attempt = 0U; attempt < I2C_IT_RETRY; attempt++)
    {
        if (attempt == 0U)
        {
            /* A previous transfer may have left the peripheral mid-transfer:
             * toggling PE resets the state machine (BUSY/MSL/...). */
            if ((g_i2c_hw.instance->SR2 & (I2C_SR2_BUSY | I2C_SR2_MSL)) != 0U)
            {
                __HAL_I2C_DISABLE(&g_i2c_handle);
                __HAL_I2C_ENABLE(&g_i2c_handle);
            }
        }
        else
        {
            i2c_hw_bus_reset();
        }

        g_it.widx  = 0U;
        g_it.ridx  = 0U;
        g_it.event = 0U;
        g_it.done  = false;
        g_it.error = false;
        g_it.phase = (g_it.kind == I2C_IT_RX) ? 1U : 0U;

        CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_POS | I2C_CR1_START | I2C_CR1_STOP);
        NVIC_ClearPendingIRQ(I2C2_EV_IRQn);
        NVIC_ClearPendingIRQ(I2C2_ER_IRQn);
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);
        __HAL_I2C_ENABLE_IT(&g_i2c_handle, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);

        if (i2c_it_wait())
        {
            return true;
        }
    }

    return false;
}

/* MEM register-prefix state machine (mirrors I2C_MemoryTransmit_TXE_BTF): send
 * the address bytes, then a repeated start, then flush. Driven by a TXE or BTF
 * event. */
static void i2c_proto_mem_prefix(void)
{
    if (g_it.event == 0U)
    {
        g_i2c_hw.instance->DR = g_it.wbuf[0];
        g_it.widx  = 1U;
        g_it.event = (g_it.wlen == 2U) ? 1U : 2U;
    }
    else if (g_it.event == 1U)
    {
        g_i2c_hw.instance->DR = g_it.wbuf[1];
        g_it.widx  = 2U;
        g_it.event = 2U;
    }
    else if (g_it.event == 2U)
    {
        g_it.phase = 1U;
        g_it.event = 3U;
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);
    }
    else
    {
        (void)g_i2c_hw.instance->SR1;   /* flush BTF/TXE */
        (void)g_i2c_hw.instance->DR;
        (void)g_i2c_hw.instance->SR1;
        (void)g_i2c_hw.instance->DR;
    }
}

/* ===== protocol engine =====
 * One set of phase handlers shared by the poll / interrupt / DMA transports.
 * The flag order and the per-count read tail follow the interrupt
 * implementation (i2c_proto_rxne / i2c_proto_rx_btf). */

static void i2c_proto_sb(void)
{
    uint8_t rw = i2c_is_read() ? 1U : 0U;

    g_i2c_hw.instance->DR = i2c_ll_dev(g_it.addr7, rw);
}

/* Per-count ACK/POS policy for a master read (shared with the DMA transport). */
static void i2c_proto_addr_flags(uint16_t n)
{
    if (n == 1U)
    {
        CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
    }
    else if (n == 2U)
    {
        CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_POS);
    }
    else
    {
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
    }
}

static void i2c_proto_addr(void)
{
    if (i2c_is_read())
    {
        uint16_t n = (uint16_t)(g_it.rlen - g_it.ridx);

        if (n == 0U)
        {
            __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);
            SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
            i2c_done();
        }
        else
        {
            i2c_proto_addr_flags(n);
            __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);
        }
    }
    else
    {
        __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);
    }
}

static void i2c_proto_tx_txe(void)
{
    if (g_it.widx < g_it.wlen)
    {
        g_i2c_hw.instance->DR = g_it.wbuf[g_it.widx++];
    }
    else
    {
        __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_BUF);
    }
}

static void i2c_proto_tx_btf(void)
{
    if (g_it.widx < g_it.wlen)
    {
        g_i2c_hw.instance->DR = g_it.wbuf[g_it.widx++];
    }
    else
    {
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
        i2c_done();
    }
}

static void i2c_proto_error(void)
{
    uint32_t err = (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR);

    if ((g_i2c_hw.instance->SR1 & err) != 0U)
    {
        g_i2c_hw.instance->SR1 = (uint16_t)~err;
        SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
        __HAL_I2C_DISABLE_IT(&g_i2c_handle, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
        g_it.error = true;
        g_it.done  = true;
    }
}

/* Route one bus event (snapshot of SR1/SR2) to the phase handlers. */
static void i2c_proto_step(uint32_t sr1, uint32_t sr2)
{
    if (g_it.done)
    {
        return;
    }
    if ((sr1 & (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR)) != 0U)
    {
        i2c_proto_error();
    }
    else if ((sr1 & I2C_SR1_SB) != 0U)
    {
        i2c_proto_sb();
    }
    else if ((sr1 & I2C_SR1_ADDR) != 0U)
    {
        i2c_proto_addr();
    }
    else if ((sr2 & I2C_SR2_TRA) != 0U)   /* transmitter */
    {
        if (g_it.kind == I2C_IT_MEMR)
        {
            i2c_proto_mem_prefix();   /* driven by a TXE or BTF event */
        }
        else if (((sr1 & I2C_SR1_TXE) != 0U) && ((sr1 & I2C_SR1_BTF) == 0U))
        {
            i2c_proto_tx_txe();
        }
        else if ((sr1 & I2C_SR1_BTF) != 0U)
        {
            i2c_proto_tx_btf();
        }
    }
    else   /* receiver */
    {
        if (((sr1 & I2C_SR1_RXNE) != 0U) && ((sr1 & I2C_SR1_BTF) == 0U))
        {
            i2c_proto_rxne();
        }
        else if ((sr1 & I2C_SR1_BTF) != 0U)
        {
            i2c_proto_rx_btf();
        }
    }
}

void I2C2_EV_IRQHandler(void)
{
    uint32_t sr1 = g_i2c_hw.instance->SR1;
    uint32_t sr2 = g_i2c_hw.instance->SR2;

    i2c_proto_step(sr1, sr2);
}

void I2C2_ER_IRQHandler(void)
{
    i2c_proto_error();
}

/* ===================================================================== */
/*                       hardware I2C2 DMA transport                     */
/* ===================================================================== */

/* Complete/abort a stream: record the flags, then stop and reset the handle
 * (HAL_DMA_Abort clears the flags and sets State back to READY so the next
 * HAL_DMA_Start does not return HAL_BUSY). */
static void i2c_dma_stream_irq(uint8_t dir)
{
    DMA_HandleTypeDef *hdma = &g_i2c_dma[dir];
    uint32_t tc = (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma)) != RESET) ? 1U : 0U;
    uint32_t te = (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TE_FLAG_INDEX(hdma)) != RESET) ? 1U : 0U;

    (void)HAL_DMA_Abort(hdma);

    if (tc != 0U)
    {
        g_dma_done[dir] = true;
    }
    if (te != 0U)
    {
        g_dma_err[dir]  = true;
        g_dma_done[dir] = true;
    }
}

void DMA1_Stream7_IRQHandler(void)
{
    i2c_dma_stream_irq(I2C_DMA_TX);
}

void DMA1_Stream2_IRQHandler(void)
{
    i2c_dma_stream_irq(I2C_DMA_RX);
}

static bool i2c_dma_wait(uint8_t dir)
{
    uint32_t start = HAL_GetTick();

    while (!g_dma_done[dir])
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            (void)HAL_DMA_Abort(&g_i2c_dma[dir]);
            return false;
        }
    }
    return !g_dma_err[dir];
}

/* Start a DMA transfer on @p dir into/from @p mem (address phase done). */
static bool i2c_dma_start(uint8_t dir, const uint8_t *src, uint8_t *dst, uint16_t len)
{
    uint32_t addr = (dir == I2C_DMA_TX) ? (uint32_t)src : (uint32_t)&g_i2c_hw.instance->DR;
    uint32_t mem  = (dir == I2C_DMA_TX) ? (uint32_t)&g_i2c_hw.instance->DR : (uint32_t)dst;

    g_dma_done[dir] = false;
    g_dma_err[dir]  = false;
    __HAL_DMA_CLEAR_FLAG(&g_i2c_dma[dir], __HAL_DMA_GET_TC_FLAG_INDEX(&g_i2c_dma[dir]));
    __HAL_DMA_CLEAR_FLAG(&g_i2c_dma[dir], __HAL_DMA_GET_TE_FLAG_INDEX(&g_i2c_dma[dir]));
    __HAL_DMA_ENABLE_IT(&g_i2c_dma[dir], DMA_IT_TC | DMA_IT_TE);
    return (HAL_DMA_Start(&g_i2c_dma[dir], addr, mem, len) == HAL_OK);
}

static bool i2c_dma_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    if (!i2c_hw_wait(I2C_FLAG_BUSY, RESET))
    {
        return false;
    }
    CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_POS);

    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);
    if (!i2c_hw_wait(I2C_FLAG_SB, SET))
    {
        i2c_hw_stop();
        return false;
    }
    g_i2c_hw.instance->DR = i2c_ll_dev(addr7, 0U);
    if (!i2c_hw_wait_addr())
    {
        i2c_hw_stop();
        return false;
    }

    /* Arm the DMA before releasing the address phase. */
    SET_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN);
    if (!i2c_dma_start(I2C_DMA_TX, buf, 0, len))
    {
        CLEAR_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN);
        __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);
        i2c_hw_stop();
        return false;
    }
    __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);
    if (!i2c_dma_wait(I2C_DMA_TX))
    {
        CLEAR_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN);
        i2c_hw_stop();
        return false;
    }
    (void)i2c_hw_wait(I2C_FLAG_BTF, SET);   /* last byte shifted out */
    CLEAR_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN);
    __HAL_DMA_DISABLE_IT(&g_i2c_dma[I2C_DMA_TX], DMA_IT_TC | DMA_IT_TE);
    i2c_hw_stop();
    return true;
}

/* DMA master receive (address phase done, read address ACKed, ADDR set). */
/* Arm the RX DMA stream (started before the address, like the HAL). */
static bool i2c_dma_arm_rx(uint8_t *buf, uint16_t len)
{
    g_dma_done[I2C_DMA_RX] = false;
    g_dma_err[I2C_DMA_RX]  = false;
    __HAL_DMA_CLEAR_FLAG(&g_i2c_dma[I2C_DMA_RX], __HAL_DMA_GET_TC_FLAG_INDEX(&g_i2c_dma[I2C_DMA_RX]));
    __HAL_DMA_CLEAR_FLAG(&g_i2c_dma[I2C_DMA_RX], __HAL_DMA_GET_TE_FLAG_INDEX(&g_i2c_dma[I2C_DMA_RX]));
    __HAL_DMA_ENABLE_IT(&g_i2c_dma[I2C_DMA_RX], DMA_IT_TC | DMA_IT_TE);
    return (HAL_DMA_Start(&g_i2c_dma[I2C_DMA_RX], (uint32_t)&g_i2c_hw.instance->DR,
                          (uint32_t)buf, len) == HAL_OK);
}

/* Address phase done (ADDR set): set the per-count ACK/POS/LAST, release ADDR,
 * wait for the DMA and close the transfer (mirrors I2C_Master_ADDR + the DMA
 * complete callback). */
static bool i2c_dma_rx_finish(uint16_t len)
{
    i2c_proto_addr_flags(len);
    SET_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
    __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);

    if (!i2c_dma_wait(I2C_DMA_RX))
    {
        CLEAR_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
        i2c_hw_stop();
        return false;
    }
    CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_STOP);
    CLEAR_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
    __HAL_DMA_DISABLE_IT(&g_i2c_dma[I2C_DMA_RX], DMA_IT_TC | DMA_IT_TE);
    return true;
}

static bool i2c_dma_read(uint8_t addr7, uint8_t *buf, uint16_t len)
{
    if (!i2c_hw_wait(I2C_FLAG_BUSY, RESET))
    {
        return false;
    }
    CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_POS);

    if (!i2c_dma_arm_rx(buf, len))
    {
        return false;
    }
    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
    SET_BIT(g_i2c_hw.instance->CR2, I2C_CR2_DMAEN);

    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);
    if (!i2c_hw_wait(I2C_FLAG_SB, SET))
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }
    g_i2c_hw.instance->DR = i2c_ll_dev(addr7, 1U);
    if (!i2c_hw_wait_addr())
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }
    return i2c_dma_rx_finish(len);
}

static bool i2c_dma_write_read(uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                               uint8_t *rbuf, uint16_t rlen)
{
    if ((wlen != 1U) && (wlen != 2U))
    {
        return false;
    }
    if (!i2c_hw_wait(I2C_FLAG_BUSY, RESET))
    {
        return false;
    }
    CLEAR_BIT(g_i2c_hw.instance->CR1, I2C_CR1_POS);

    if (!i2c_dma_arm_rx(rbuf, rlen))
    {
        return false;
    }

    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_ACK);
    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);
    if (!i2c_hw_wait(I2C_FLAG_SB, SET))
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }
    g_i2c_hw.instance->DR = i2c_ll_dev(addr7, 0U);
    if (!i2c_hw_wait_addr())
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }
    __HAL_I2C_CLEAR_ADDRFLAG(&g_i2c_handle);

    if (!i2c_hw_wait(I2C_FLAG_TXE, SET))
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }
    g_i2c_hw.instance->DR = wbuf[0];
    if (wlen == 2U)
    {
        if (!i2c_hw_wait(I2C_FLAG_TXE, SET))
        {
            (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
            i2c_hw_stop();
            return false;
        }
        g_i2c_hw.instance->DR = wbuf[1];
    }
    if (!i2c_hw_wait(I2C_FLAG_TXE, SET))
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }

    SET_BIT(g_i2c_hw.instance->CR1, I2C_CR1_START);   /* repeated start */
    if (!i2c_hw_wait(I2C_FLAG_SB, SET))
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }
    g_i2c_hw.instance->DR = i2c_ll_dev(addr7, 1U);
    if (!i2c_hw_wait_addr())
    {
        (void)HAL_DMA_Abort(&g_i2c_dma[I2C_DMA_RX]);
        i2c_hw_stop();
        return false;
    }
    return i2c_dma_rx_finish(rlen);
}

/* ===================================================================== */
/*                       initialisation                                  */
/* ===================================================================== */

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

    g_i2c.io       = c->io;
    g_i2c.speed_hz = (c->speed_hz != 0U) ? c->speed_hz : 100000U;

    if (g_i2c.io != I2C_IO_SW)
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

        /* DMA streams. */
        dma_hw_setup(&g_i2c_dma[I2C_DMA_TX], &g_i2c_hw.dma[I2C_DMA_TX]);
        dma_hw_setup(&g_i2c_dma[I2C_DMA_RX], &g_i2c_hw.dma[I2C_DMA_RX]);

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

    if (g_i2c.io == I2C_IO_SW)
    {
        return i2c_sw_write(addr7, buf, len);
    }
    if (len == 0U)
    {
        return i2c_hw_probe(addr7);
    }
    if (g_i2c.io == I2C_IO_IT)
    {
        return i2c_it_xfer(addr7, buf, len, 0, 0U);
    }
    if (g_i2c.io == I2C_IO_DMA)
    {
        return i2c_dma_write(addr7, buf, len);
    }
    return i2c_hw_poll(addr7, buf, len, 0, 0U);
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

    if (g_i2c.io == I2C_IO_SW)
    {
        return i2c_sw_read(addr7, buf, len);
    }
    if (g_i2c.io == I2C_IO_IT)
    {
        return i2c_it_xfer(addr7, 0, 0U, buf, len);
    }
    if (g_i2c.io == I2C_IO_DMA)
    {
        return i2c_dma_read(addr7, buf, len);
    }
    return i2c_hw_poll(addr7, 0, 0U, buf, len);
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

    if (g_i2c.io == I2C_IO_SW)
    {
        return i2c_sw_write_read(addr7, wbuf, wlen, rbuf, rlen);
    }
    if (g_i2c.io == I2C_IO_IT)
    {
        return i2c_it_xfer(addr7, wbuf, wlen, rbuf, rlen);
    }
    if (g_i2c.io == I2C_IO_DMA)
    {
        return i2c_dma_write_read(addr7, wbuf, wlen, rbuf, rlen);
    }
    return i2c_hw_poll(addr7, wbuf, wlen, rbuf, rlen);
}
