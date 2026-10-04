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

/* ===== hardware descriptor (one entry per bus) ===== */

enum { I2C_DMA_TX = 0, I2C_DMA_RX = 1 };

/* The single on-board bus that carries every device (maps device -> bus; a
 * device->bus table would replace this if a second bus is added). */
#define I2C_DEV_BUS     I2C_ID_1

typedef struct
{
    I2C_TypeDef *instance;      /*!< I2C2 */
    IRQn_Type    ev_irqn;       /*!< I2C2_EV_IRQn */
    IRQn_Type    er_irqn;       /*!< I2C2_ER_IRQn */
    uint32_t     rcc_en;        /*!< RCC_APB1ENR_I2C2EN */
    gpio_hw_t    scl;           /*!< PH4 */
    gpio_hw_t    sda;           /*!< PH5 */
    dma_hw_t     dma[2];        /*!< [0] TX, [1] RX (8-bit, normal mode) */
} i2c_hw_t;

static const i2c_hw_t g_hw[I2C_ID_NUM] =
{
    [I2C_ID_1] = {
        .instance = I2C2,
        .ev_irqn  = I2C2_EV_IRQn,
        .er_irqn  = I2C2_ER_IRQn,
        .rcc_en   = RCC_APB1ENR_I2C2EN,
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
    },
};

/* ===== per-bus state ===== */

/* Master transfer kinds (shared by the poll / interrupt transports). */
enum { I2C_IT_TX = 0, I2C_IT_RX, I2C_IT_MEMR };

/* One runtime handle per bus: configuration, HAL objects, DMA flags and the
 * current transfer context (kind/phase/buffers/counters). */
typedef struct
{
    i2c_id_t          id;
    i2c_io_t          io;
    uint32_t          speed_hz;
    bool              ready;
    const i2c_hw_t   *hw;
    I2C_HandleTypeDef hi2c;
    DMA_HandleTypeDef hdma[2];
    volatile bool     dma_done[2];
    volatile bool     dma_err[2];

    uint8_t        kind;      /* TX / RX / MEMR */
    uint8_t        addr7;
    uint8_t        phase;     /* MEMR: 0 = prefix write, 1 = read */
    uint8_t        event;     /* MEMR prefix: HAL-style EventCount */
    const uint8_t *wbuf;
    uint16_t       wlen, widx;
    uint8_t       *rbuf;
    uint16_t       rlen, ridx;
    volatile bool  done;
    volatile bool  error;
} i2c_handle_t;

static i2c_handle_t g_i2c[I2C_ID_NUM];

#define I2C_IT_RETRY   3U   /* re-try a transaction that left the bus stuck */

/* 7-bit address + R/W bit -> the byte written to DR. */
#define I2C_ADDR7(addr7, rw)   (uint8_t)(((addr7) << 1) | ((rw) & 1U))

/* ===================================================================== */
/*                       software bit-bang backend                       */
/* ===================================================================== */

/* The software bit-bang runs at a fixed 100 kHz (5 us half-period). */
#define I2C_SW_HALF_US  5U

#define SW_SCL(x)   HAL_GPIO_WritePin(h->hw->scl.port, h->hw->scl.pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SW_SDA(x)   HAL_GPIO_WritePin(h->hw->sda.port, h->hw->sda.pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SW_SDA_RD   HAL_GPIO_ReadPin(h->hw->sda.port, h->hw->sda.pin)

static void i2c_sw_start(i2c_handle_t *h)
{
    SW_SDA(1);
    SW_SCL(1);
    delay_us(I2C_SW_HALF_US);
    SW_SDA(0);
    delay_us(I2C_SW_HALF_US);
    SW_SCL(0);
    delay_us(I2C_SW_HALF_US);
}

static void i2c_sw_stop(i2c_handle_t *h)
{
    SW_SDA(0);
    delay_us(I2C_SW_HALF_US);
    SW_SCL(1);
    delay_us(I2C_SW_HALF_US);
    SW_SDA(1);
    delay_us(I2C_SW_HALF_US);
}

static void i2c_sw_send(i2c_handle_t *h, uint8_t data)
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

static uint8_t i2c_sw_wait_ack(i2c_handle_t *h)
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
            i2c_sw_stop(h);
            return 1U;
        }
        delay_us(I2C_SW_HALF_US);
    }
    SW_SCL(0);
    delay_us(I2C_SW_HALF_US);
    return 0U;
}

static uint8_t i2c_sw_read_byte(i2c_handle_t *h, uint8_t ack)
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

static bool i2c_sw_write(i2c_handle_t *h, uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    i2c_sw_start(h);
    i2c_sw_send(h, I2C_ADDR7(addr7, 0U));
    if (i2c_sw_wait_ack(h) != 0U)
    {
        i2c_sw_stop(h);
        return false;
    }
    for (i = 0U; i < len; i++)
    {
        i2c_sw_send(h, buf[i]);
        if (i2c_sw_wait_ack(h) != 0U)
        {
            i2c_sw_stop(h);
            return false;
        }
    }
    i2c_sw_stop(h);
    return true;
}

static bool i2c_sw_read(i2c_handle_t *h, uint8_t addr7, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    i2c_sw_start(h);
    i2c_sw_send(h, I2C_ADDR7(addr7, 1U));
    if (i2c_sw_wait_ack(h) != 0U)
    {
        i2c_sw_stop(h);
        return false;
    }
    for (i = 0U; i < len; i++)
    {
        buf[i] = i2c_sw_read_byte(h, (i == (uint16_t)(len - 1U)) ? 0U : 1U);
    }
    i2c_sw_stop(h);
    return true;
}

static bool i2c_sw_write_read(i2c_handle_t *h, uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                              uint8_t *rbuf, uint16_t rlen)
{
    uint16_t i;

    i2c_sw_start(h);
    i2c_sw_send(h, I2C_ADDR7(addr7, 0U));
    if (i2c_sw_wait_ack(h) != 0U)
    {
        i2c_sw_stop(h);
        return false;
    }
    for (i = 0U; i < wlen; i++)
    {
        i2c_sw_send(h, wbuf[i]);
        if (i2c_sw_wait_ack(h) != 0U)
        {
            i2c_sw_stop(h);
            return false;
        }
    }

    i2c_sw_start(h);   /* repeated start */
    i2c_sw_send(h, I2C_ADDR7(addr7, 1U));
    if (i2c_sw_wait_ack(h) != 0U)
    {
        i2c_sw_stop(h);
        return false;
    }
    for (i = 0U; i < rlen; i++)
    {
        rbuf[i] = i2c_sw_read_byte(h, (i == (uint16_t)(rlen - 1U)) ? 0U : 1U);
    }
    i2c_sw_stop(h);
    return true;
}

/* ===================================================================== */
/*                       hardware I2C2 backend (flag macros)             */
/* ===================================================================== */

/* Shared protocol engine (defined with the interrupt transport below). */
static void i2c_proto_step(i2c_handle_t *h, uint32_t sr1, uint32_t sr2);

/* Recover a bus held low by a stuck slave, then re-init the peripheral and
 * re-enable it. Used as the retry step by the poll / interrupt paths. */
static void i2c_hw_reset(i2c_handle_t *h)
{
    gpio_hw_t scl = h->hw->scl;
    gpio_hw_t sda = h->hw->sda;
    uint8_t   i;

    __HAL_I2C_DISABLE(&h->hi2c);

    /* Clock SCL 9x with SDA released, then a STOP, to free the bus. */
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

    gpio_hw_setup(&h->hw->scl);
    gpio_hw_setup(&h->hw->sda);
    (void)HAL_I2C_Init(&h->hi2c);
    SET_BIT(h->hw->instance->CR1, I2C_CR1_PE);
}

/* Release the hardware transport (STOP, interrupts off, DMA aborted) before a
 * transfer is discarded or the bus is re-configured. */
static void i2c_abort(i2c_handle_t *h)
{
    if (h->io == I2C_IO_SW)
    {
        return;
    }
    SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
    __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
    (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_TX]);
    (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
}

/* Wait until __FLAG__ == WANT (SET/RESET). false on timeout. */
static bool i2c_hw_wait(i2c_handle_t *h, uint32_t flag, FlagStatus want)
{
    uint32_t start = HAL_GetTick();

    while (__HAL_I2C_GET_FLAG(&h->hi2c, flag) != want)
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            return false;
        }
    }
    return true;
}

/* Wait for the address phase (ADDR set, or NACK with AF). Leaves ADDR set. */
static bool i2c_hw_wait_addr(i2c_handle_t *h)
{
    uint32_t start = HAL_GetTick();

    while ((__HAL_I2C_GET_FLAG(&h->hi2c, I2C_FLAG_ADDR) == RESET) &&
           (__HAL_I2C_GET_FLAG(&h->hi2c, I2C_FLAG_AF) == RESET))
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            return false;
        }
    }
    if (__HAL_I2C_GET_FLAG(&h->hi2c, I2C_FLAG_AF) == SET)
    {
        __HAL_I2C_CLEAR_FLAG(&h->hi2c, I2C_FLAG_AF);
        return false;
    }
    return true;
}

static void i2c_hw_stop(i2c_handle_t *h)
{
    SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
    __HAL_I2C_CLEAR_FLAG(&h->hi2c, I2C_FLAG_AF);
}

/* Probe: is the device present at @p addr7? */
static bool i2c_hw_probe(i2c_handle_t *h, uint8_t addr7)
{
    SET_BIT(h->hw->instance->CR1, I2C_CR1_START);
    if (!i2c_hw_wait(h, I2C_FLAG_SB, SET))
    {
        i2c_hw_stop(h);
        return false;
    }
    h->hw->instance->DR = I2C_ADDR7(addr7, 0U);
    if (!i2c_hw_wait_addr(h))
    {
        i2c_hw_stop(h);
        return false;
    }
    __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);
    i2c_hw_stop(h);
    return true;
}

/* Generic polled transfer: set the context up, start, then drive the shared
 * protocol engine from the main loop (exactly the events the IT ISR feeds), so
 * the poll / interrupt / DMA transports share one read tail and MEM prefix. */
static bool i2c_hw_poll(i2c_handle_t *h, uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                        uint8_t *rbuf, uint16_t rlen)
{
    uint8_t attempt;

    h->addr7 = addr7;
    h->wbuf  = wbuf;
    h->wlen  = wlen;
    h->rbuf  = rbuf;
    h->rlen  = rlen;
    if (rlen == 0U)
    {
        h->kind = I2C_IT_TX;
    }
    else if (wlen == 0U)
    {
        h->kind = I2C_IT_RX;
    }
    else
    {
        h->kind = I2C_IT_MEMR;
    }

    for (attempt = 0U; attempt < I2C_IT_RETRY; attempt++)
    {
        uint32_t start;

        if (attempt == 0U)
        {
            /* A previous transfer may have left the peripheral mid-transfer. */
            if ((h->hw->instance->SR2 & (I2C_SR2_BUSY | I2C_SR2_MSL)) != 0U)
            {
                __HAL_I2C_DISABLE(&h->hi2c);
                __HAL_I2C_ENABLE(&h->hi2c);
            }
        }
        else
        {
            i2c_hw_reset(h);
        }

        h->widx  = 0U;
        h->ridx  = 0U;
        h->event = 0U;
        h->done  = false;
        h->error = false;
        h->phase = (h->kind == I2C_IT_RX) ? 1U : 0U;

        if (!i2c_hw_wait(h, I2C_FLAG_BUSY, RESET))
        {
            continue;
        }
        CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_POS);
        SET_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
        SET_BIT(h->hw->instance->CR1, I2C_CR1_START);

        start = HAL_GetTick();
        while (!h->done)
        {
            uint32_t sr1;
            uint32_t sr2;

            if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
            {
                i2c_hw_stop(h);
                break;
            }
            sr1 = h->hw->instance->SR1;   /* read SR1 before SR2 (clears ADDR) */
            sr2 = h->hw->instance->SR2;
            i2c_proto_step(h, sr1, sr2);
        }
        if (h->done && !h->error)
        {
            return true;
        }
    }
    return false;
}

/* ===================================================================== */
/*                       hardware I2C2 interrupt transport               */
/* ===================================================================== */

/* Read tail (mirrors I2C_MasterReceive_RXNE/BTF); @p btf selects the BTF event.
 * The read address must already be ACKed (ADDR cleared). */
static void i2c_proto_rx(i2c_handle_t *h, bool btf)
{
    uint16_t n = (uint16_t)(h->rlen - h->ridx);

    if (!btf)
    {
        if (n > 3U)
        {
            h->rbuf[h->ridx++] = (uint8_t)h->hw->instance->DR;
            if ((uint16_t)(h->rlen - h->ridx) == 3U)
            {
                __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_BUF);
            }
        }
        else if (n == 1U)
        {
            CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
            SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
            h->rbuf[h->ridx++] = (uint8_t)h->hw->instance->DR;
            __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
            h->done = true;
        }
        else
        {
            __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_BUF);
        }
    }
    else if (n == 4U)
    {
        __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_BUF);
        h->rbuf[h->ridx++] = (uint8_t)h->hw->instance->DR;
    }
    else if (n == 3U)
    {
        __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_BUF);
        CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
        h->rbuf[h->ridx++] = (uint8_t)h->hw->instance->DR;
    }
    else if (n == 2U)
    {
        SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
        h->rbuf[h->ridx++] = (uint8_t)h->hw->instance->DR;
        h->rbuf[h->ridx++] = (uint8_t)h->hw->instance->DR;
        __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
        h->done = true;
    }
    else
    {
        h->rbuf[h->ridx++] = (uint8_t)h->hw->instance->DR;
    }
}

static bool i2c_it_xfer(i2c_handle_t *h, uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                        uint8_t *rbuf, uint16_t rlen)
{
    uint8_t attempt;

    h->addr7 = addr7;
    h->wbuf  = wbuf;
    h->wlen  = wlen;
    h->rbuf  = rbuf;
    h->rlen  = rlen;

    if (rlen == 0U)
    {
        h->kind  = I2C_IT_TX;
        h->phase = 0U;
    }
    else if (wlen == 0U)
    {
        h->kind  = I2C_IT_RX;
        h->phase = 1U;
    }
    else
    {
        h->kind  = I2C_IT_MEMR;
        h->phase = 0U;
    }

    for (attempt = 0U; attempt < I2C_IT_RETRY; attempt++)
    {
        if (attempt == 0U)
        {
            /* A previous transfer may have left the peripheral mid-transfer:
             * toggling PE resets the state machine (BUSY/MSL/...). */
            if ((h->hw->instance->SR2 & (I2C_SR2_BUSY | I2C_SR2_MSL)) != 0U)
            {
                __HAL_I2C_DISABLE(&h->hi2c);
                __HAL_I2C_ENABLE(&h->hi2c);
            }
        }
        else
        {
            i2c_hw_reset(h);
        }

        h->widx  = 0U;
        h->ridx  = 0U;
        h->event = 0U;
        h->done  = false;
        h->error = false;
        h->phase = (h->kind == I2C_IT_RX) ? 1U : 0U;

        CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_POS | I2C_CR1_START | I2C_CR1_STOP);
        NVIC_ClearPendingIRQ(h->hw->ev_irqn);
        NVIC_ClearPendingIRQ(h->hw->er_irqn);
        SET_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
        SET_BIT(h->hw->instance->CR1, I2C_CR1_START);
        __HAL_I2C_ENABLE_IT(&h->hi2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);

        {
            uint32_t start = HAL_GetTick();

            while (!h->done)
            {
                if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
                {
                    SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
                    __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
                    break;
                }
            }
        }
        if (h->done && !h->error)
        {
            return true;
        }
    }

    return false;
}

/* Per-count ACK/POS policy for a master read (also used by the DMA transport). */
static void i2c_proto_addr_flags(i2c_handle_t *h, uint16_t n)
{
    if (n == 1U)
    {
        CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
    }
    else if (n == 2U)
    {
        CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
        SET_BIT(h->hw->instance->CR1, I2C_CR1_POS);
    }
    else
    {
        SET_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
    }
}

/* Route one bus event (snapshot of SR1/SR2) to the master phase handling. */
static void i2c_proto_step(i2c_handle_t *h, uint32_t sr1, uint32_t sr2)
{
    uint32_t err = (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR);
    bool     read;

    if (h->done)
    {
        return;
    }
    read = (h->kind == I2C_IT_RX) || ((h->kind == I2C_IT_MEMR) && (h->phase != 0U));

    if ((sr1 & err) != 0U)
    {
        h->hw->instance->SR1 = (uint16_t)~err;
        SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
        __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
        h->error = true;
        h->done  = true;
    }
    else if ((sr1 & I2C_SR1_SB) != 0U)          /* start sent -> address */
    {
        h->hw->instance->DR = I2C_ADDR7(h->addr7, read ? 1U : 0U);
    }
    else if ((sr1 & I2C_SR1_ADDR) != 0U)
    {
        if (read)
        {
            uint16_t n = (uint16_t)(h->rlen - h->ridx);

            if (n != 0U)
            {
                i2c_proto_addr_flags(h, n);
                __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);
            }
            else
            {
                __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);
                SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
                h->done = true;
            }
        }
        else
        {
            __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);
        }
    }
    else if ((sr2 & I2C_SR2_TRA) != 0U)          /* transmitter */
    {
        if (h->kind == I2C_IT_MEMR)
        {
            /* Memory-address prefix: send the 1/2 bytes, then repeat-start. */
            if (h->event == 0U)
            {
                h->hw->instance->DR = h->wbuf[0];
                h->widx  = 1U;
                h->event = (h->wlen == 2U) ? 1U : 2U;
            }
            else if (h->event == 1U)
            {
                h->hw->instance->DR = h->wbuf[1];
                h->widx  = 2U;
                h->event = 2U;
            }
            else if (h->event == 2U)
            {
                h->phase = 1U;
                h->event = 3U;
                SET_BIT(h->hw->instance->CR1, I2C_CR1_START);
            }
            else
            {
                (void)h->hw->instance->SR1;   /* flush BTF/TXE */
                (void)h->hw->instance->DR;
                (void)h->hw->instance->SR1;
                (void)h->hw->instance->DR;
            }
        }
        else if (((sr1 & I2C_SR1_TXE) != 0U) && ((sr1 & I2C_SR1_BTF) == 0U))
        {
            if (h->widx < h->wlen)
            {
                h->hw->instance->DR = h->wbuf[h->widx++];
            }
            else
            {
                __HAL_I2C_DISABLE_IT(&h->hi2c, I2C_IT_BUF);
            }
        }
        else if ((sr1 & I2C_SR1_BTF) != 0U)
        {
            if (h->widx < h->wlen)
            {
                h->hw->instance->DR = h->wbuf[h->widx++];
            }
            else
            {
                SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
                h->done = true;
            }
        }
    }
    else                                          /* receiver */
    {
        if (((sr1 & I2C_SR1_RXNE) != 0U) && ((sr1 & I2C_SR1_BTF) == 0U))
        {
            i2c_proto_rx(h, false);
        }
        else if ((sr1 & I2C_SR1_BTF) != 0U)
        {
            i2c_proto_rx(h, true);
        }
    }
}

void I2C2_EV_IRQHandler(void)
{
    i2c_handle_t *h = &g_i2c[I2C_DEV_BUS];
    uint32_t      sr1 = h->hw->instance->SR1;
    uint32_t      sr2 = h->hw->instance->SR2;

    i2c_proto_step(h, sr1, sr2);
}

void I2C2_ER_IRQHandler(void)
{
    i2c_handle_t *h = &g_i2c[I2C_DEV_BUS];

    i2c_proto_step(h, h->hw->instance->SR1, h->hw->instance->SR2);
}

/* ===================================================================== */
/*                       hardware I2C2 DMA transport                     */
/* ===================================================================== */

/* Complete/abort a stream: record the flags, then stop and reset the handle
 * (HAL_DMA_Abort clears the flags and sets State back to READY so the next
 * HAL_DMA_Start does not return HAL_BUSY). */
static void i2c_dma_stream_irq(i2c_handle_t *h, uint8_t dir)
{
    DMA_HandleTypeDef *hdma = &h->hdma[dir];
    uint32_t tc = (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma)) != RESET) ? 1U : 0U;
    uint32_t te = (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TE_FLAG_INDEX(hdma)) != RESET) ? 1U : 0U;

    (void)HAL_DMA_Abort(hdma);

    if (tc != 0U)
    {
        h->dma_done[dir] = true;
    }
    if (te != 0U)
    {
        h->dma_err[dir]  = true;
        h->dma_done[dir] = true;
    }
}

void DMA1_Stream7_IRQHandler(void)
{
    i2c_dma_stream_irq(&g_i2c[I2C_DEV_BUS], I2C_DMA_TX);
}

void DMA1_Stream2_IRQHandler(void)
{
    i2c_dma_stream_irq(&g_i2c[I2C_DEV_BUS], I2C_DMA_RX);
}

static bool i2c_dma_wait(i2c_handle_t *h, uint8_t dir)
{
    uint32_t start = HAL_GetTick();

    while (!h->dma_done[dir])
    {
        if ((HAL_GetTick() - start) >= I2C_TIMEOUT_MS)
        {
            (void)HAL_DMA_Abort(&h->hdma[dir]);
            return false;
        }
    }
    return !h->dma_err[dir];
}

/* Start a DMA transfer on @p dir from the handle's current buffers. */
static bool i2c_dma_start(i2c_handle_t *h, uint8_t dir)
{
    uint32_t addr = (dir == I2C_DMA_TX) ? (uint32_t)h->wbuf : (uint32_t)&h->hw->instance->DR;
    uint32_t mem  = (dir == I2C_DMA_TX) ? (uint32_t)&h->hw->instance->DR : (uint32_t)h->rbuf;
    uint16_t len  = (dir == I2C_DMA_TX) ? h->wlen : h->rlen;

    h->dma_done[dir] = false;
    h->dma_err[dir]  = false;
    __HAL_DMA_CLEAR_FLAG(&h->hdma[dir], __HAL_DMA_GET_TC_FLAG_INDEX(&h->hdma[dir]));
    __HAL_DMA_CLEAR_FLAG(&h->hdma[dir], __HAL_DMA_GET_TE_FLAG_INDEX(&h->hdma[dir]));
    __HAL_DMA_ENABLE_IT(&h->hdma[dir], DMA_IT_TC | DMA_IT_TE);
    return (HAL_DMA_Start(&h->hdma[dir], addr, mem, len) == HAL_OK);
}

/* Address phase done (ADDR set): per-count ACK/POS/LAST, release ADDR, wait for
 * the RX DMA and close the transfer (mirrors I2C_Master_ADDR + DMA complete). */
static bool i2c_dma_rx_finish(i2c_handle_t *h)
{
    i2c_proto_addr_flags(h, h->rlen);
    SET_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
    __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);

    if (!i2c_dma_wait(h, I2C_DMA_RX))
    {
        CLEAR_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
        i2c_hw_stop(h);
        return false;
    }
    CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
    SET_BIT(h->hw->instance->CR1, I2C_CR1_STOP);
    CLEAR_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
    __HAL_DMA_DISABLE_IT(&h->hdma[I2C_DMA_RX], DMA_IT_TC | DMA_IT_TE);
    return true;
}

/* DMA transfer: @p rlen == 0 -> write, @p wlen == 0 -> read, else a memory read
 * with a 1/2-byte address prefix.  The address phase is polled. */
static bool i2c_dma_xfer(i2c_handle_t *h, uint8_t addr7, const uint8_t *wbuf, uint16_t wlen,
                         uint8_t *rbuf, uint16_t rlen)
{
    if ((rlen != 0U) && (wlen != 0U) && (wlen != 1U) && (wlen != 2U))
    {
        return false;
    }
    h->addr7 = addr7;
    h->wbuf  = wbuf;
    h->wlen  = wlen;
    h->rbuf  = rbuf;
    h->rlen  = rlen;

    if (!i2c_hw_wait(h, I2C_FLAG_BUSY, RESET))
    {
        return false;
    }
    CLEAR_BIT(h->hw->instance->CR1, I2C_CR1_POS);

    if (rlen == 0U)
    {
        /* Write: arm the TX DMA before releasing the address phase. */
        SET_BIT(h->hw->instance->CR1, I2C_CR1_START);
        if (!i2c_hw_wait(h, I2C_FLAG_SB, SET))
        {
            i2c_hw_stop(h);
            return false;
        }
        h->hw->instance->DR = I2C_ADDR7(addr7, 0U);
        if (!i2c_hw_wait_addr(h))
        {
            i2c_hw_stop(h);
            return false;
        }
        SET_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN);
        if (!i2c_dma_start(h, I2C_DMA_TX))
        {
            CLEAR_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN);
            __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);
            i2c_hw_stop(h);
            return false;
        }
        __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);
        if (!i2c_dma_wait(h, I2C_DMA_TX))
        {
            CLEAR_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN);
            i2c_hw_stop(h);
            return false;
        }
        (void)i2c_hw_wait(h, I2C_FLAG_BTF, SET);   /* last byte shifted out */
        CLEAR_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN);
        __HAL_DMA_DISABLE_IT(&h->hdma[I2C_DMA_TX], DMA_IT_TC | DMA_IT_TE);
        i2c_hw_stop(h);
        return true;
    }

    /* Read: the RX DMA is armed before the address. */
    if (!i2c_dma_start(h, I2C_DMA_RX))
    {
        return false;
    }
    SET_BIT(h->hw->instance->CR1, I2C_CR1_ACK);
    SET_BIT(h->hw->instance->CR2, I2C_CR2_DMAEN);
    SET_BIT(h->hw->instance->CR1, I2C_CR1_START);
    if (!i2c_hw_wait(h, I2C_FLAG_SB, SET))
    {
        (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
        i2c_hw_stop(h);
        return false;
    }
    h->hw->instance->DR = I2C_ADDR7(addr7, (wlen == 0U) ? 1U : 0U);
    if (!i2c_hw_wait_addr(h))
    {
        (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
        i2c_hw_stop(h);
        return false;
    }
    if (wlen == 0U)
    {
        return i2c_dma_rx_finish(h);
    }

    /* Memory read: send the 1/2-byte address, then repeat-start. */
    __HAL_I2C_CLEAR_ADDRFLAG(&h->hi2c);
    if (!i2c_hw_wait(h, I2C_FLAG_TXE, SET))
    {
        (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
        i2c_hw_stop(h);
        return false;
    }
    h->hw->instance->DR = wbuf[0];
    if (wlen == 2U)
    {
        if (!i2c_hw_wait(h, I2C_FLAG_TXE, SET))
        {
            (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
            i2c_hw_stop(h);
            return false;
        }
        h->hw->instance->DR = wbuf[1];
    }
    if (!i2c_hw_wait(h, I2C_FLAG_TXE, SET))
    {
        (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
        i2c_hw_stop(h);
        return false;
    }
    SET_BIT(h->hw->instance->CR1, I2C_CR1_START);
    if (!i2c_hw_wait(h, I2C_FLAG_SB, SET))
    {
        (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
        i2c_hw_stop(h);
        return false;
    }
    h->hw->instance->DR = I2C_ADDR7(addr7, 1U);
    if (!i2c_hw_wait_addr(h))
    {
        (void)HAL_DMA_Abort(&h->hdma[I2C_DMA_RX]);
        i2c_hw_stop(h);
        return false;
    }
    return i2c_dma_rx_finish(h);
}

/* ===================================================================== */
/*                       initialisation                                  */
/* ===================================================================== */


void i2c_init(const i2c_cfg_t *cfg)
{
    static const i2c_cfg_t cfg_default = { I2C_CFG_DEFAULT(I2C_ID_1) };
    const i2c_cfg_t *c = (cfg != 0) ? cfg : &cfg_default;
    i2c_handle_t    *h;
    uint32_t         speed;

    if ((c->id >= I2C_ID_NUM) || ((cfg == 0) && g_i2c[c->id].ready))
    {
        return;   /* unknown bus, or a NULL cfg after the first initialisation */
    }
    h = &g_i2c[c->id];
    speed = (c->speed_hz != 0U) ? c->speed_hz : 100000U;

    if (h->ready && (h->io == c->io) && (h->speed_hz == speed))
    {
        return;   /* same configuration: nothing to do */
    }

    h->id = c->id;
    h->hw = &g_hw[c->id];
    if (h->ready)
    {
        i2c_abort(h);   /* release the previous transport before re-configuring */
    }
    h->io       = c->io;
    h->speed_hz = speed;

    if (h->io == I2C_IO_SW)
    {
        gpio_hw_t scl = h->hw->scl;
        gpio_hw_t sda = h->hw->sda;

        scl.mode      = GPIO_MODE_OUTPUT_PP;
        scl.alternate = 0U;
        sda.mode      = GPIO_MODE_OUTPUT_OD;
        sda.alternate = 0U;
        gpio_hw_setup(&scl);
        gpio_hw_setup(&sda);
        i2c_sw_stop(h);
    }
    else
    {
        /* Hardware I2C2: configure first, then recover the bus only if it is
         * actually held (BUSY) - never unconditionally before init. */
        SET_BIT(RCC->APB1ENR, h->hw->rcc_en);   /* was __HAL_RCC_I2C2_CLK_ENABLE() */
        gpio_hw_setup(&h->hw->scl);
        gpio_hw_setup(&h->hw->sda);

        h->hi2c.Instance             = h->hw->instance;
        h->hi2c.Init.ClockSpeed      = h->speed_hz;
        h->hi2c.Init.DutyCycle       = (c->duty_cycle == I2C_DUTY_16_9) ? I2C_DUTYCYCLE_16_9 : I2C_DUTYCYCLE_2;
        h->hi2c.Init.OwnAddress1     = c->own_address1;
        h->hi2c.Init.AddressingMode  = (c->addressing_mode == I2C_ADDR_10BIT) ? I2C_ADDRESSINGMODE_10BIT : I2C_ADDRESSINGMODE_7BIT;
        h->hi2c.Init.DualAddressMode = (c->dual_address_mode == I2C_DUAL_ENABLE) ? I2C_DUALADDRESS_ENABLE : I2C_DUALADDRESS_DISABLE;
        h->hi2c.Init.OwnAddress2     = c->own_address2;
        h->hi2c.Init.GeneralCallMode = (c->general_call_mode == I2C_GCALL_ENABLE) ? I2C_GENERALCALL_ENABLE : I2C_GENERALCALL_DISABLE;
        h->hi2c.Init.NoStretchMode   = (c->stretch == I2C_STRETCH_DISABLE) ? I2C_NOSTRETCH_ENABLE : I2C_NOSTRETCH_DISABLE;
        (void)HAL_I2C_Init(&h->hi2c);

        if ((h->hw->instance->SR2 & I2C_SR2_BUSY) != 0U)
        {
            i2c_hw_reset(h);   /* the bus is held by a stuck slave */
        }

        /* DMA streams + NVIC (events/errors + the two streams). */
        dma_hw_setup(&h->hdma[I2C_DMA_TX], &h->hw->dma[I2C_DMA_TX]);
        dma_hw_setup(&h->hdma[I2C_DMA_RX], &h->hw->dma[I2C_DMA_RX]);

        HAL_NVIC_SetPriority(h->hw->ev_irqn, c->irq_preempt, c->irq_sub);
        HAL_NVIC_EnableIRQ(h->hw->ev_irqn);
        HAL_NVIC_SetPriority(h->hw->er_irqn, c->irq_preempt, c->irq_sub);
        HAL_NVIC_EnableIRQ(h->hw->er_irqn);
        HAL_NVIC_SetPriority(h->hw->dma[I2C_DMA_TX].irqn, c->irq_preempt, c->irq_sub);
        HAL_NVIC_EnableIRQ(h->hw->dma[I2C_DMA_TX].irqn);
        HAL_NVIC_SetPriority(h->hw->dma[I2C_DMA_RX].irqn, c->irq_preempt, c->irq_sub);
        HAL_NVIC_EnableIRQ(h->hw->dma[I2C_DMA_RX].irqn);
    }

    h->ready = true;
}

/* ===================================================================== */
/*                       public API                                      */
/* ===================================================================== */

bool i2c_write(i2c_device_t dev, const uint8_t *buf, uint16_t len)
{
    i2c_handle_t *h = &g_i2c[I2C_DEV_BUS];
    uint8_t       addr7;

    if (!h->ready || (dev >= I2C_DEV_NUM))
    {
        return false;
    }
    addr7 = g_i2c_dev_addr[dev];

    if (h->io == I2C_IO_SW)
    {
        return i2c_sw_write(h, addr7, buf, len);
    }
    if (len == 0U)
    {
        return i2c_hw_probe(h, addr7);
    }
    if (h->io == I2C_IO_IT)
    {
        return i2c_it_xfer(h, addr7, buf, len, 0, 0U);
    }
    if (h->io == I2C_IO_DMA)
    {
        return i2c_dma_xfer(h, addr7, buf, len, 0, 0U);
    }
    return i2c_hw_poll(h, addr7, buf, len, 0, 0U);
}

bool i2c_read(i2c_device_t dev, uint8_t *buf, uint16_t len)
{
    i2c_handle_t *h = &g_i2c[I2C_DEV_BUS];
    uint8_t       addr7;

    if (!h->ready || (dev >= I2C_DEV_NUM))
    {
        return false;
    }
    if (len == 0U)
    {
        return true;   /* nothing to receive */
    }
    addr7 = g_i2c_dev_addr[dev];

    if (h->io == I2C_IO_SW)
    {
        return i2c_sw_read(h, addr7, buf, len);
    }
    if (h->io == I2C_IO_IT)
    {
        return i2c_it_xfer(h, addr7, 0, 0U, buf, len);
    }
    if (h->io == I2C_IO_DMA)
    {
        return i2c_dma_xfer(h, addr7, 0, 0U, buf, len);
    }
    return i2c_hw_poll(h, addr7, 0, 0U, buf, len);
}

bool i2c_write_read(i2c_device_t dev,
                    const uint8_t *wbuf, uint16_t wlen,
                    uint8_t *rbuf, uint16_t rlen)
{
    i2c_handle_t *h = &g_i2c[I2C_DEV_BUS];
    uint8_t       addr7;

    if (!h->ready || (dev >= I2C_DEV_NUM))
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

    if (h->io == I2C_IO_SW)
    {
        return i2c_sw_write_read(h, addr7, wbuf, wlen, rbuf, rlen);
    }
    if (h->io == I2C_IO_IT)
    {
        return i2c_it_xfer(h, addr7, wbuf, wlen, rbuf, rlen);
    }
    if (h->io == I2C_IO_DMA)
    {
        return i2c_dma_xfer(h, addr7, wbuf, wlen, rbuf, rlen);
    }
    return i2c_hw_poll(h, addr7, wbuf, wlen, rbuf, rlen);
}
