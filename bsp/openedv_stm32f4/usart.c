/**
 * @file    usart.c
 * @brief   Unified USART1/USART2 driver (id based). Transport only: poll /
 *          interrupt / DMA are selected through usart_cfg_t; line parsing is
 *          left to the application. TX goes through HAL; RX IT uses HAL's
 *          1-byte reception re-armed in the completion callback; RX DMA uses a
 *          circular transfer drained on the USART IDLE interrupt (no DMA stream
 *          IRQ is used, so it cannot clash with other users of that stream).
 */

#include "stm32f4xx_hal.h"
#include "usart.h"

/* ===== constants / DMA streams ===== */

#define USART_TX_TIMEOUT_MS   1000U
#define USART_DMA_IRQ_PREEMPT 3U
#define USART_DMA_IRQ_SUB     3U

/* USART1: TX = DMA2_Stream7/ch4, RX = DMA2_Stream2/ch4. */
#define USART1_TX_DMA_STREAM   DMA2_Stream7
#define USART1_RX_DMA_STREAM   DMA2_Stream2
#define USART_TX_DMA_CHANNEL   DMA_CHANNEL_4
#define USART_RX_DMA_CHANNEL   DMA_CHANNEL_4
#define USART1_TX_DMA_IRQn     DMA2_Stream7_IRQn

/* USART2: TX = DMA1_Stream6/ch4, RX = DMA1_Stream5/ch4 (shared with DAC; the
 * IDLE-driven RX does not request the stream IRQ, so there is no symbol clash). */
#define USART2_TX_DMA_STREAM   DMA1_Stream6
#define USART2_RX_DMA_STREAM   DMA1_Stream5
#define USART2_TX_DMA_IRQn     DMA1_Stream6_IRQn

/* ===== context ===== */

typedef struct
{
    UART_HandleTypeDef    huart;
    DMA_HandleTypeDef     hdma_tx;
    DMA_HandleTypeDef     hdma_rx;
    usart_io_t            tx;
    usart_io_t            rx;
    uint8_t              *buf;
    uint16_t              size;
    volatile uint16_t     head;   /* write index (DMA position after IDLE) */
    volatile uint16_t     tail;   /* read index (usart_read) */
    volatile usart_rx_cb_t cb;
    uint8_t               rx_byte; /* landing byte for HAL RX IT */
} usart_handle_t;

static usart_handle_t g_uart[USART_ID_NUM];

static usart_handle_t *usart_handle_of(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        return &g_uart[USART_ID_1];
    }
    if (huart->Instance == USART2)
    {
        return &g_uart[USART_ID_2];
    }
    return 0;
}

/* ===== DMA setup ===== */

static void usart_dma_tx_init(usart_handle_t *handle, usart_id_t id)
{
    if (id == USART_ID_1)
    {
        __HAL_RCC_DMA2_CLK_ENABLE();
        handle->hdma_tx.Instance = USART1_TX_DMA_STREAM;
    }
    else
    {
        __HAL_RCC_DMA1_CLK_ENABLE();
        handle->hdma_tx.Instance = USART2_TX_DMA_STREAM;
    }

    handle->hdma_tx.Init.Channel             = USART_TX_DMA_CHANNEL;
    handle->hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    handle->hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    handle->hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    handle->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    handle->hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    handle->hdma_tx.Init.Mode                = DMA_NORMAL;
    handle->hdma_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    handle->hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    __HAL_LINKDMA(&handle->huart, hdmatx, handle->hdma_tx);

    HAL_DMA_DeInit(&handle->hdma_tx);
    (void)HAL_DMA_Init(&handle->hdma_tx);

    HAL_NVIC_SetPriority((id == USART_ID_1) ? USART1_TX_DMA_IRQn : USART2_TX_DMA_IRQn,
                         USART_DMA_IRQ_PREEMPT, USART_DMA_IRQ_SUB);
    HAL_NVIC_EnableIRQ((id == USART_ID_1) ? USART1_TX_DMA_IRQn : USART2_TX_DMA_IRQn);
}

/* RX DMA: circular transfer into handle->buf; bytes are handed over on the
 * USART IDLE interrupt. The DMA stream IRQ is intentionally not enabled. */
static void usart_dma_rx_init(usart_handle_t *handle, usart_id_t id)
{
    if (id == USART_ID_1)
    {
        __HAL_RCC_DMA2_CLK_ENABLE();
        handle->hdma_rx.Instance = USART1_RX_DMA_STREAM;
    }
    else
    {
        __HAL_RCC_DMA1_CLK_ENABLE();
        handle->hdma_rx.Instance = USART2_RX_DMA_STREAM;
    }

    handle->hdma_rx.Init.Channel             = USART_RX_DMA_CHANNEL;
    handle->hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    handle->hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    handle->hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    handle->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    handle->hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    handle->hdma_rx.Init.Mode                = DMA_CIRCULAR;
    handle->hdma_rx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    handle->hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    __HAL_LINKDMA(&handle->huart, hdmarx, handle->hdma_rx);

    HAL_DMA_DeInit(&handle->hdma_rx);
    (void)HAL_DMA_Init(&handle->hdma_rx);

    __HAL_UART_ENABLE_IT(&handle->huart, UART_IT_IDLE);
    (void)HAL_UART_Receive_DMA(&handle->huart, handle->buf, handle->size);
}

/* ===== receive paths ===== */

/* Hand the bytes written by the circular DMA since the last IDLE to the callback. */
static void usart_dma_idle(usart_handle_t *handle)
{
    uint16_t pos = (uint16_t)(handle->size - __HAL_DMA_GET_COUNTER(&handle->hdma_rx));

    while (handle->head != pos)
    {
        uint8_t byte = handle->buf[handle->head];

        if (handle->cb != 0)
        {
            handle->cb(byte);
        }
        handle->head = (uint16_t)((handle->head + 1U) % handle->size);
        if (handle->head == handle->tail)
        {
            break; /* full: drop the rest until read drains */
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    usart_handle_t *handle = usart_handle_of(huart);
    uint8_t         byte;

    if ((handle == 0) || (handle->rx != USART_IO_IT))
    {
        return; /* DMA reception is drained through the IDLE handler */
    }

    byte = handle->rx_byte;
    if (handle->cb != 0)
    {
        handle->cb(byte);
    }
    if ((handle->buf != 0) && (handle->size != 0U))
    {
        uint16_t next = (uint16_t)((handle->head + 1U) % handle->size);

        if (next != handle->tail)
        {
            handle->buf[handle->head] = byte;
            handle->head = next;
        }
    }

    (void)HAL_UART_Receive_IT(huart, &handle->rx_byte, 1U);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    usart_handle_t *handle = usart_handle_of(huart);

    if (handle == 0)
    {
        return;
    }

    __HAL_UART_CLEAR_OREFLAG(huart);
    if (handle->rx == USART_IO_IT)
    {
        (void)HAL_UART_Receive_IT(huart, &handle->rx_byte, 1U);
    }
    else if (handle->rx == USART_IO_DMA)
    {
        (void)HAL_UART_Receive_DMA(huart, handle->buf, handle->size);
    }
}

/* ===== public API ===== */

void usart_init(const usart_cfg_t *cfg)
{
    usart_handle_t  *handle;
    GPIO_InitTypeDef gpio_init = {0};
    usart_io_t       tx;
    usart_io_t       rx;
    IRQn_Type        irqn;

    if (cfg->id >= USART_ID_NUM)
    {
        return;
    }
    handle = &g_uart[cfg->id];
    tx = cfg->tx;
    rx = cfg->rx;

    /* Stop any reception left armed by a previous initialisation. */
    if (handle->huart.Instance != 0)
    {
        (void)HAL_UART_AbortReceive(&handle->huart);
        __HAL_UART_DISABLE_IT(&handle->huart, UART_IT_RXNE | UART_IT_IDLE);
    }

    /* RX IT/DMA needs a buffer (M1). */
    if (((rx == USART_IO_IT) || (rx == USART_IO_DMA)) &&
        ((cfg->rx_buf == 0) || (cfg->rx_size == 0U)))
    {
        rx = USART_IO_POLL;
    }

    handle->tx   = tx;
    handle->rx   = rx;
    handle->buf  = cfg->rx_buf;
    handle->size = cfg->rx_size;
    handle->head = 0U;
    handle->tail = 0U;

    /* ---- MSP begin: clocks + GPIO AF + NVIC ---- */
    if (cfg->id == USART_ID_1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        handle->huart.Instance = USART1;
        gpio_init.Alternate    = GPIO_AF7_USART1;
        gpio_init.Pin          = GPIO_PIN_9 | GPIO_PIN_10;
        irqn                   = USART1_IRQn;
    }
    else
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        handle->huart.Instance = USART2;
        gpio_init.Alternate    = GPIO_AF7_USART2;
        gpio_init.Pin          = GPIO_PIN_2 | GPIO_PIN_3;
        irqn                   = USART2_IRQn;
    }
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Mode  = GPIO_MODE_AF_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    if ((tx != USART_IO_POLL) || (rx != USART_IO_POLL))
    {
        HAL_NVIC_SetPriority(irqn, cfg->irq_preempt, cfg->irq_sub);
        HAL_NVIC_EnableIRQ(irqn);
    }
    /* ---- MSP end ---- */

    handle->huart.Init.BaudRate     = cfg->baudrate;
    handle->huart.Init.WordLength   = cfg->word_length;
    handle->huart.Init.StopBits     = cfg->stop_bits;
    handle->huart.Init.Parity       = cfg->parity;
    handle->huart.Init.Mode         = cfg->mode;
    handle->huart.Init.HwFlowCtl    = cfg->hw_flow_ctl;
    handle->huart.Init.OverSampling = cfg->oversampling;
    (void)HAL_UART_Init(&handle->huart);

    if (tx == USART_IO_DMA)
    {
        usart_dma_tx_init(handle, cfg->id);
    }

    if (rx == USART_IO_IT)
    {
        (void)HAL_UART_Receive_IT(&handle->huart, &handle->rx_byte, 1U);
    }
    else if (rx == USART_IO_DMA)
    {
        usart_dma_rx_init(handle, cfg->id);
    }
}

void usart_set_rx_cb(usart_id_t id, usart_rx_cb_t cb)
{
    g_uart[id].cb = cb;
}

bool usart_tx_busy(usart_id_t id)
{
    return (g_uart[id].huart.gState != HAL_UART_STATE_READY);
}

bool usart_write(usart_id_t id, const uint8_t *data, uint32_t len)
{
    usart_handle_t *handle = &g_uart[id];
    uint16_t        n;

    if ((data == 0) || (len == 0U) || usart_tx_busy(id))
    {
        return false;
    }
    n = (len > 0xFFFFU) ? 0xFFFFU : (uint16_t)len;

    if (handle->tx == USART_IO_IT)
    {
        return (HAL_UART_Transmit_IT(&handle->huart, (uint8_t *)data, n) == HAL_OK);
    }
    if (handle->tx == USART_IO_DMA)
    {
        return (HAL_UART_Transmit_DMA(&handle->huart, (uint8_t *)data, n) == HAL_OK);
    }
    return (HAL_UART_Transmit(&handle->huart, (uint8_t *)data, n, USART_TX_TIMEOUT_MS) == HAL_OK);
}

uint32_t usart_read(usart_id_t id, uint8_t *data, uint32_t len, uint32_t timeout)
{
    usart_handle_t *handle = &g_uart[id];
    uint32_t        n = 0U;
    uint32_t        start = HAL_GetTick();

    while (n < len)
    {
        if ((handle->rx == USART_IO_IT) || (handle->rx == USART_IO_DMA))
        {
            if (handle->tail != handle->head)
            {
                data[n++] = handle->buf[handle->tail];
                handle->tail = (uint16_t)((handle->tail + 1U) % handle->size);
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

/* ===== interrupts ===== */

static void usart_irq(usart_handle_t *handle)
{
    HAL_UART_IRQHandler(&handle->huart);

    if ((handle->rx == USART_IO_DMA) &&
        (__HAL_UART_GET_FLAG(&handle->huart, UART_FLAG_IDLE) != RESET))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&handle->huart);
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

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_uart[USART_ID_1].hdma_tx);
}

void DMA1_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_uart[USART_ID_2].hdma_tx);
}

/* ===== stdio ===== */

/**
 * @brief  newlib-nano stdout sink: send one byte over USART1 (blocking).
 */
int __io_putchar(int ch)
{
    uint8_t byte = (uint8_t)ch;

    (void)HAL_UART_Transmit(&g_uart[USART_ID_1].huart, &byte, 1U, USART_TX_TIMEOUT_MS);

    return ch;
}
