/**
 * @file    usart.c
 * @brief   Unified USART1/USART2 driver (id based). Transport only: poll /
 *          interrupt / DMA are selected through usart_cfg_t; line parsing is
 *          left to the application.
 *
 * The interrupt paths are handled explicitly (RXNE / TXE / TC / ORE / IDLE plus
 * the TX DMA stream TC), so the HAL is used only for peripheral/stream
 * initialisation (HAL_UART_Init / HAL_DMA_Init) and never for IRQ dispatch.
 * RX IT reads DR directly; RX DMA is a circular transfer drained on the USART
 * IDLE interrupt (the DMA stream IRQ is not used, so it cannot clash with other
 * users of that stream).
 */

#include "stm32f4xx_hal.h"
#include "usart.h"
#include "ringbuf.h"
#include "gpio_hw.h"
#include "dma_hw.h"

/* ===== constants ===== */

#define USART_TX_MAX_WORD 0xFFFFU /* max bytes per IT/DMA transfer (16-bit NDTR) */
#define USART_TX_TIMEOUT_MS 100U  /* per-byte POLL wait budget (ms) */

/* ===== hardware descriptors ===== */

/* USART1: TX = DMA2_Stream7/ch4, RX = DMA2_Stream2/ch4.
 * USART2: TX = DMA1_Stream6/ch4, RX = DMA1_Stream5/ch4 (the IDLE-driven RX does
 * not request the stream IRQ, so Stream5 can be shared with the DAC). */
typedef struct
{
    USART_TypeDef *instance;
    IRQn_Type      irqn;     /* USARTx_IRQn */
    gpio_hw_t      gpio_tx;  /* TX pin (AF push-pull) */
    gpio_hw_t      gpio_rx;  /* RX pin (AF push-pull) */
    dma_hw_t       tx_dma;   /* MEMORY_TO_PERIPH, DMA_NORMAL */
    dma_hw_t       rx_dma;   /* PERIPH_TO_MEMORY, DMA_CIRCULAR (IDLE-drained) */
} usart_hw_t;

static const usart_hw_t g_hw[USART_ID_NUM] =
{
    {
        .instance = USART1, .irqn = USART1_IRQn,
        .gpio_tx  = { GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_9,  GPIO_MODE_AF_PP,
                      GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART1 },
        .gpio_rx  = { GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_10, GPIO_MODE_AF_PP,
                      GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART1 },
        .tx_dma   = {
            .rcc_en = RCC_AHB1ENR_DMA2EN, .stream = DMA2_Stream7, .irqn = DMA2_Stream7_IRQn,
            .channel = DMA_CHANNEL_4, .direction = DMA_MEMORY_TO_PERIPH,
            .periph_inc = DMA_PINC_DISABLE, .mem_inc = DMA_MINC_ENABLE,
            .periph_align = DMA_PDATAALIGN_BYTE, .mem_align = DMA_MDATAALIGN_BYTE,
            .mode = DMA_NORMAL, .priority = DMA_PRIORITY_MEDIUM,
            .fifo_mode = DMA_FIFOMODE_DISABLE, .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst = DMA_MBURST_SINGLE, .periph_burst = DMA_PBURST_SINGLE,
        },
        .rx_dma   = {
            .rcc_en = RCC_AHB1ENR_DMA2EN, .stream = DMA2_Stream2, .irqn = DMA2_Stream2_IRQn,
            .channel = DMA_CHANNEL_4, .direction = DMA_PERIPH_TO_MEMORY,
            .periph_inc = DMA_PINC_DISABLE, .mem_inc = DMA_MINC_ENABLE,
            .periph_align = DMA_PDATAALIGN_BYTE, .mem_align = DMA_MDATAALIGN_BYTE,
            .mode = DMA_CIRCULAR, .priority = DMA_PRIORITY_MEDIUM,
            .fifo_mode = DMA_FIFOMODE_DISABLE, .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst = DMA_MBURST_SINGLE, .periph_burst = DMA_PBURST_SINGLE,
        },
    },
    {
        .instance = USART2, .irqn = USART2_IRQn,
        .gpio_tx  = { GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_2, GPIO_MODE_AF_PP,
                      GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART2 },
        .gpio_rx  = { GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_3, GPIO_MODE_AF_PP,
                      GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART2 },
        .tx_dma   = {
            .rcc_en = RCC_AHB1ENR_DMA1EN, .stream = DMA1_Stream6, .irqn = DMA1_Stream6_IRQn,
            .channel = DMA_CHANNEL_4, .direction = DMA_MEMORY_TO_PERIPH,
            .periph_inc = DMA_PINC_DISABLE, .mem_inc = DMA_MINC_ENABLE,
            .periph_align = DMA_PDATAALIGN_BYTE, .mem_align = DMA_MDATAALIGN_BYTE,
            .mode = DMA_NORMAL, .priority = DMA_PRIORITY_MEDIUM,
            .fifo_mode = DMA_FIFOMODE_DISABLE, .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst = DMA_MBURST_SINGLE, .periph_burst = DMA_PBURST_SINGLE,
        },
        .rx_dma   = {
            .rcc_en = RCC_AHB1ENR_DMA1EN, .stream = DMA1_Stream5, .irqn = DMA1_Stream5_IRQn,
            .channel = DMA_CHANNEL_4, .direction = DMA_PERIPH_TO_MEMORY,
            .periph_inc = DMA_PINC_DISABLE, .mem_inc = DMA_MINC_ENABLE,
            .periph_align = DMA_PDATAALIGN_BYTE, .mem_align = DMA_MDATAALIGN_BYTE,
            .mode = DMA_CIRCULAR, .priority = DMA_PRIORITY_MEDIUM,
            .fifo_mode = DMA_FIFOMODE_DISABLE, .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst = DMA_MBURST_SINGLE, .periph_burst = DMA_PBURST_SINGLE,
        },
    },
};

/* ===== context ===== */

typedef struct
{
    UART_HandleTypeDef    huart;
    DMA_HandleTypeDef     hdma_tx;
    DMA_HandleTypeDef     hdma_rx;
    usart_io_t            tx;
    usart_io_t            rx;
    ringbuf_t             rb;      /* RX ring (head = DMA position after IDLE) */
    volatile usart_rx_cb_t cb;
    volatile bool         tx_busy; /* asynchronous (IT/DMA) transmit in progress */
    const uint8_t        *volatile tx_ptr; /* TX IT cursor */
    volatile uint16_t     tx_len;  /* TX IT bytes left */
} usart_handle_t;

static usart_handle_t g_uart[USART_ID_NUM];

/* ===== frame option mapping (own enum -> HAL) ===== */

static uint32_t usart_word_to_hal(usart_word_len_t v)
{
    return (v == USART_WORD_9B) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
}

static uint32_t usart_stop_to_hal(usart_stop_bits_t v)
{
    return (v == USART_STOP_2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
}

static uint32_t usart_parity_to_hal(usart_parity_t v)
{
    switch (v)
    {
        case USART_PAR_EVEN: return UART_PARITY_EVEN;
        case USART_PAR_ODD:  return UART_PARITY_ODD;
        default:             return UART_PARITY_NONE;
    }
}

static uint32_t usart_mode_to_hal(usart_mode_t v)
{
    switch (v)
    {
        case USART_DIR_TX: return UART_MODE_TX;
        case USART_DIR_RX: return UART_MODE_RX;
        default:           return UART_MODE_TX_RX;
    }
}

static uint32_t usart_flow_to_hal(usart_flow_t v)
{
    switch (v)
    {
        case USART_FLOW_RTS:     return UART_HWCONTROL_RTS;
        case USART_FLOW_CTS:     return UART_HWCONTROL_CTS;
        case USART_FLOW_RTS_CTS: return UART_HWCONTROL_RTS_CTS;
        default:                 return UART_HWCONTROL_NONE;
    }
}

static uint32_t usart_oversampling_to_hal(usart_oversampling_t v)
{
    return (v == USART_OS_8) ? UART_OVERSAMPLING_8 : UART_OVERSAMPLING_16;
}

/* ===== DMA setup ===== */

static void usart_dma_tx_init(usart_handle_t *handle, const usart_hw_t *hw,
                              const usart_cfg_t *cfg)
{
    dma_hw_setup(&handle->hdma_tx, &hw->tx_dma);
    __HAL_LINKDMA(&handle->huart, hdmatx, handle->hdma_tx);

    HAL_NVIC_SetPriority(hw->tx_dma.irqn, cfg->irq_preempt, cfg->irq_sub);
    HAL_NVIC_EnableIRQ(hw->tx_dma.irqn);
}

/* RX DMA: circular transfer into handle->rb.buf; bytes are handed over on the
 * USART IDLE interrupt. The DMA stream IRQ is intentionally not enabled. */
static void usart_dma_rx_init(usart_handle_t *handle, const usart_hw_t *hw)
{
    dma_hw_setup(&handle->hdma_rx, &hw->rx_dma);
    __HAL_LINKDMA(&handle->huart, hdmarx, handle->hdma_rx);

    /* Start the circular transfer; only IDLE is used to drain it. */
    SET_BIT(handle->huart.Instance->CR3, USART_CR3_DMAR);
    __HAL_UART_ENABLE_IT(&handle->huart, UART_IT_IDLE);
    (void)HAL_DMA_Start(&handle->hdma_rx, (uint32_t)&handle->huart.Instance->DR,
                        (uint32_t)handle->rb.buf, handle->rb.size);
}

/* ===== transmit paths ===== */

/* Blocking transmit with a per-byte timeout; returns false when it stalls. */
static bool usart_tx_wait(usart_handle_t *handle, const uint8_t *data, uint16_t len)
{
    uint32_t start;
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        start = HAL_GetTick();
        while (__HAL_UART_GET_FLAG(&handle->huart, UART_FLAG_TXE) == RESET)
        {
            if ((HAL_GetTick() - start) >= USART_TX_TIMEOUT_MS)
            {
                return false;
            }
        }
        handle->huart.Instance->DR = data[i];
    }

    start = HAL_GetTick();
    while (__HAL_UART_GET_FLAG(&handle->huart, UART_FLAG_TC) == RESET)
    {
        if ((HAL_GetTick() - start) >= USART_TX_TIMEOUT_MS)
        {
            return false;
        }
    }
    __HAL_UART_CLEAR_FLAG(&handle->huart, UART_FLAG_TC);
    return true;
}

/* ===== receive paths ===== */

/* Deliver one byte: callback first, then the ring buffer (drop when full). */
static bool usart_rx_deliver(usart_handle_t *handle, uint8_t byte)
{
    if (handle->cb != 0)
    {
        handle->cb(byte);
    }
    return ringbuf_put(&handle->rb, byte);
}

/* Hand the bytes written by the circular DMA since the last IDLE to the callback. */
static void usart_dma_idle(usart_handle_t *handle)
{
    uint16_t pos = (uint16_t)(handle->rb.size - __HAL_DMA_GET_COUNTER(&handle->hdma_rx));

    while (handle->rb.head != pos)
    {
        if (!usart_rx_deliver(handle, handle->rb.buf[handle->rb.head]))
        {
            break; /* full: drop the rest until read drains */
        }
    }
}

/* ===== public API ===== */

void usart_init(const usart_cfg_t *cfg)
{
    usart_handle_t   *handle;
    const usart_hw_t *hw;
    usart_io_t        tx;
    usart_io_t        rx;

    if (cfg->id >= USART_ID_NUM)
    {
        return;
    }
    handle = &g_uart[cfg->id];
    hw     = &g_hw[cfg->id];
    tx = cfg->tx;
    rx = cfg->rx;

    /* Stop any activity left armed by a previous initialisation. */
    if (handle->huart.Instance != 0)
    {
        __HAL_UART_DISABLE_IT(&handle->huart, UART_IT_RXNE | UART_IT_TXE |
                                              UART_IT_TC | UART_IT_IDLE);
        CLEAR_BIT(handle->huart.Instance->CR3, USART_CR3_DMAT | USART_CR3_DMAR);
        if (handle->hdma_tx.Instance != 0)
        {
            (void)HAL_DMA_Abort(&handle->hdma_tx);
        }
        if (handle->hdma_rx.Instance != 0)
        {
            (void)HAL_DMA_Abort(&handle->hdma_rx);
        }
    }

    /* RX IT/DMA needs a buffer (M1). */
    if (((rx == USART_IO_IT) || (rx == USART_IO_DMA)) &&
        ((cfg->rx_buf == 0) || (cfg->rx_size == 0U)))
    {
        rx = USART_IO_POLL;
    }

    handle->tx      = tx;
    handle->rx      = rx;
    ringbuf_init(&handle->rb, cfg->rx_buf, cfg->rx_size);
    handle->tx_busy = false;
    handle->tx_ptr  = 0;
    handle->tx_len  = 0U;

    /* ---- MSP begin: clocks + GPIO AF + NVIC ---- */
    if (cfg->id == USART_ID_1) { __HAL_RCC_USART1_CLK_ENABLE(); }
    else                       { __HAL_RCC_USART2_CLK_ENABLE(); }
    handle->huart.Instance = hw->instance;
    gpio_hw_setup(&hw->gpio_tx);
    gpio_hw_setup(&hw->gpio_rx);

    if ((tx != USART_IO_POLL) || (rx != USART_IO_POLL))
    {
        HAL_NVIC_SetPriority(hw->irqn, cfg->irq_preempt, cfg->irq_sub);
        HAL_NVIC_EnableIRQ(hw->irqn);
    }
    /* ---- MSP end ---- */

    handle->huart.Init.BaudRate     = cfg->baudrate;
    handle->huart.Init.WordLength   = usart_word_to_hal(cfg->word_length);
    handle->huart.Init.StopBits     = usart_stop_to_hal(cfg->stop_bits);
    handle->huart.Init.Parity       = usart_parity_to_hal(cfg->parity);
    handle->huart.Init.Mode         = usart_mode_to_hal(cfg->mode);
    handle->huart.Init.HwFlowCtl    = usart_flow_to_hal(cfg->hw_flow_ctl);
    handle->huart.Init.OverSampling = usart_oversampling_to_hal(cfg->oversampling);
    (void)HAL_UART_Init(&handle->huart);

    if (tx == USART_IO_DMA)
    {
        usart_dma_tx_init(handle, hw, cfg);
    }

    if (rx == USART_IO_IT)
    {
        __HAL_UART_ENABLE_IT(&handle->huart, UART_IT_RXNE);
    }
    else if (rx == USART_IO_DMA)
    {
        usart_dma_rx_init(handle, hw);
    }
}

void usart_set_rx_cb(usart_id_t id, usart_rx_cb_t cb)
{
    if (id >= USART_ID_NUM)
    {
        return;
    }
    g_uart[id].cb = cb;
}

bool usart_tx_busy(usart_id_t id)
{
    if (id >= USART_ID_NUM)
    {
        return true;
    }
    return g_uart[id].tx_busy;
}

bool usart_write(usart_id_t id, const uint8_t *data, uint32_t len)
{
    usart_handle_t *handle;
    uint16_t        n;

    if ((id >= USART_ID_NUM) || (data == 0) || (len == 0U) ||
        (len > USART_TX_MAX_WORD) || usart_tx_busy(id))
    {
        return false;
    }
    handle = &g_uart[id];
    n = (uint16_t)len;

    if (handle->tx == USART_IO_IT)
    {
        __HAL_UART_CLEAR_FLAG(&handle->huart, UART_FLAG_TC);
        handle->tx_ptr  = data;
        handle->tx_len  = n;
        handle->tx_busy = true;
        __HAL_UART_ENABLE_IT(&handle->huart, UART_IT_TXE);
        return true;
    }
    else if (handle->tx == USART_IO_DMA)
    {
        __HAL_DMA_CLEAR_FLAG(&handle->hdma_tx, __HAL_DMA_GET_TC_FLAG_INDEX(&handle->hdma_tx));
        __HAL_UART_CLEAR_FLAG(&handle->huart, UART_FLAG_TC);
        __HAL_UART_DISABLE_IT(&handle->huart, UART_IT_TC);
        handle->tx_busy = true;

        SET_BIT(handle->huart.Instance->CR3, USART_CR3_DMAT);
        if (HAL_DMA_Start(&handle->hdma_tx, (uint32_t)data,
                          (uint32_t)&handle->huart.Instance->DR, n) != HAL_OK)
        {
            CLEAR_BIT(handle->huart.Instance->CR3, USART_CR3_DMAT);
            handle->tx_busy = false;
            return false;
        }
        __HAL_DMA_ENABLE_IT(&handle->hdma_tx, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME);
        return true;
    }
    else if (handle->tx == USART_IO_POLL)
    {
        if (!usart_tx_wait(handle, data, n))
        {
            return false;
        }
        return true;
    }

    return false; /* unknown transport mode */
}

uint32_t usart_read(usart_id_t id, uint8_t *data, uint32_t len, uint32_t timeout)
{
    usart_handle_t *handle;
    uint32_t        n = 0U;
    uint32_t        start;

    if (id >= USART_ID_NUM)
    {
        return 0U;
    }
    handle = &g_uart[id];
    start = HAL_GetTick();

    while (n < len)
    {
        if ((handle->rx == USART_IO_IT) || (handle->rx == USART_IO_DMA))
        {
            if (ringbuf_get(&handle->rb, &data[n]))
            {
                n++;
                start = HAL_GetTick(); /* timeout is per byte (inter-byte) */
                continue;
            }
        }
        else
        {
            if (__HAL_UART_GET_FLAG(&handle->huart, UART_FLAG_RXNE) != RESET)
            {
                data[n++] = (uint8_t)handle->huart.Instance->DR;
                start = HAL_GetTick();
                continue;
            }
            if (__HAL_UART_GET_FLAG(&handle->huart, UART_FLAG_ORE) != RESET)
            {
                __HAL_UART_CLEAR_OREFLAG(&handle->huart);
            }
        }

        if (timeout == 0U)
        {
            break;
        }
        if ((HAL_GetTick() - start) >= timeout)
        {
            break;
        }
    }

    return n;
}

/* ===== interrupts (explicit flags, no HAL IRQ dispatch) ===== */

static void usart_irq(usart_handle_t *handle)
{
    UART_HandleTypeDef *huart = &handle->huart;

    /* TX interrupt mode: feed the next byte, then wait for TC to release. */
    if ((__HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE) != RESET) &&
        (__HAL_UART_GET_FLAG(huart, UART_FLAG_TXE) != RESET))
    {
        if (handle->tx_len != 0U)
        {
            huart->Instance->DR = *handle->tx_ptr;
            handle->tx_ptr++;
            handle->tx_len--;
        }
        if (handle->tx_len == 0U)
        {
            __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
            __HAL_UART_ENABLE_IT(huart, UART_IT_TC);
        }
    }

    /* TX complete (DMA completion also waits here for the last bit). */
    if ((__HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC) != RESET) &&
        (__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) != RESET))
    {
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);
        __HAL_UART_DISABLE_IT(huart, UART_IT_TC);
        CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAT);
        handle->tx_busy = false;
    }

    /* RX interrupt mode: read DR directly (also clears RXNE/ORE). */
    if ((handle->rx == USART_IO_IT) &&
        (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) != RESET))
    {
        (void)usart_rx_deliver(handle, (uint8_t)huart->Instance->DR);
    }

    /* Overrun: clear it so reception continues (RX IT/DMA only). */
    if ((handle->rx != USART_IO_POLL) &&
        (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE) != RESET))
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
    }

    /* DMA RX is drained on IDLE (no stream IRQ). */
    if ((handle->rx == USART_IO_DMA) &&
        (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) != RESET))
    {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        usart_dma_idle(handle);
        /* Drop any latched DMA transfer-error flag (no stream IRQ is used). */
        __HAL_DMA_CLEAR_FLAG(&handle->hdma_rx, __HAL_DMA_GET_TE_FLAG_INDEX(&handle->hdma_rx));
    }
}

void USART1_IRQHandler(void)
{
    usart_irq(&g_uart[USART_ID_1]);
}

void USART2_IRQHandler(void)
{
    usart_irq(&g_uart[USART_ID_2]);
}

static void usart_dma_tx_irq(usart_handle_t *handle)
{
    DMA_HandleTypeDef *hdma = &handle->hdma_tx;
    bool               tc   = (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma)) != RESET);

    (void)HAL_DMA_Abort(hdma);

    if (tc)
    {
        __HAL_UART_ENABLE_IT(&handle->huart, UART_IT_TC); /* wait for the shifter */
    }
    else
    {
        /* TE/FE/DME: abort and release the busy flag. */
        CLEAR_BIT(handle->huart.Instance->CR3, USART_CR3_DMAT);
        handle->tx_busy = false;
    }
}

void DMA2_Stream7_IRQHandler(void)
{
    usart_dma_tx_irq(&g_uart[USART_ID_1]);
}

void DMA1_Stream6_IRQHandler(void)
{
    usart_dma_tx_irq(&g_uart[USART_ID_2]);
}

/* ===== stdio ===== */

/**
 * @brief  newlib-nano stdout sink: send one byte over USART1 (blocking).
 * @note   Waits for any in-flight asynchronous transmit to finish first so the
 *         byte stream is not interleaved.
 */
int __io_putchar(int ch)
{
    usart_handle_t *handle = &g_uart[USART_ID_1];
    uint8_t         byte   = (uint8_t)ch;

    while (handle->tx_busy)
    {
    }
    (void)usart_tx_wait(handle, &byte, 1U);

    return ch;
}
