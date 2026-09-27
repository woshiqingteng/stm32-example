/**
 * @file    usart.c
 * @brief   Unified USART1/USART2 driver (id based). Transport only: poll /
 *          interrupt / DMA are selected through usart_cfg_t; line parsing is
 *          left to the application. TX goes through HAL; RX IT uses HAL's
 *          1-byte reception re-armed in the completion callback.
 */

#include "stm32f4xx_hal.h"
#include "usart.h"

#define USART_DATA_MASK     0xFFU
#define USART_TX_TIMEOUT_MS 1000U
#define USART_DMA_IRQ_PREEMPT 3U
#define USART_DMA_IRQ_SUB     3U

/* DMA2_Stream7 / channel 4 is USART1_TX. */
#define USART1_TX_DMA_STREAM   DMA2_Stream7
#define USART1_TX_DMA_CHANNEL  DMA_CHANNEL_4
#define USART1_TX_DMA_IRQn     DMA2_Stream7_IRQn

typedef struct
{
    UART_HandleTypeDef    huart;
    DMA_HandleTypeDef     hdma_tx;
    DMA_HandleTypeDef     hdma_rx;    /* reserved: USART1 RX DMA (phase 2) */
    usart_io_t            tx;
    usart_io_t            rx;
    uint8_t              *buf;
    uint16_t              size;
    volatile uint16_t     head;
    volatile uint16_t     tail;
    volatile usart_rx_cb_t cb;
    uint8_t               rx_byte;    /* landing byte for HAL RX IT */
} usart_ctx_t;

static usart_ctx_t g_uart[USART_ID_NUM];

static usart_ctx_t *usart_ctx_of(UART_HandleTypeDef *huart)
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

static void usart_store_byte(usart_ctx_t *ctx, uint8_t byte)
{
    if (ctx->cb != 0)
    {
        ctx->cb(byte);
    }

    if ((ctx->buf != 0) && (ctx->size != 0U))
    {
        uint16_t next = (uint16_t)((ctx->head + 1U) % ctx->size);

        if (next != ctx->tail)
        {
            ctx->buf[ctx->head] = byte;
            ctx->head = next;
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    usart_ctx_t *ctx = usart_ctx_of(huart);

    if (ctx == 0)
    {
        return;
    }

    usart_store_byte(ctx, ctx->rx_byte);
    (void)HAL_UART_Receive_IT(huart, &ctx->rx_byte, 1U);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    usart_ctx_t *ctx = usart_ctx_of(huart);

    if (ctx == 0)
    {
        return;
    }

    __HAL_UART_CLEAR_OREFLAG(huart);
    if (ctx->rx == USART_IO_IT)
    {
        (void)HAL_UART_Receive_IT(huart, &ctx->rx_byte, 1U);
    }
}

static void usart_dma_tx_init(usart_ctx_t *ctx)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    ctx->hdma_tx.Instance                 = USART1_TX_DMA_STREAM;
    ctx->hdma_tx.Init.Channel             = USART1_TX_DMA_CHANNEL;
    ctx->hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    ctx->hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    ctx->hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    ctx->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    ctx->hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    ctx->hdma_tx.Init.Mode                = DMA_NORMAL;
    ctx->hdma_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    ctx->hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    __HAL_LINKDMA(&ctx->huart, hdmatx, ctx->hdma_tx);

    HAL_DMA_DeInit(&ctx->hdma_tx);
    (void)HAL_DMA_Init(&ctx->hdma_tx);

    HAL_NVIC_SetPriority(USART1_TX_DMA_IRQn, USART_DMA_IRQ_PREEMPT, USART_DMA_IRQ_SUB);
    HAL_NVIC_EnableIRQ(USART1_TX_DMA_IRQn);
}

void usart_init(const usart_cfg_t *cfg)
{
    usart_ctx_t     *ctx = &g_uart[cfg->id];
    GPIO_InitTypeDef gpio_init = {0};
    usart_io_t       tx = cfg->tx;
    usart_io_t       rx = cfg->rx;
    IRQn_Type        irqn;

    /* USART2 DMA is reserved -> poll. */
    if (cfg->id == USART_ID_2)
    {
        if (tx == USART_IO_DMA) { tx = USART_IO_POLL; }
        if (rx == USART_IO_DMA) { rx = USART_IO_POLL; }
    }
    /* RX IT/DMA needs a buffer (M1). */
    if (((rx == USART_IO_IT) || (rx == USART_IO_DMA)) &&
        ((cfg->rx_buf == 0) || (cfg->rx_size == 0U)))
    {
        rx = USART_IO_POLL;
    }

    ctx->tx   = tx;
    ctx->rx   = rx;
    ctx->buf  = cfg->rx_buf;
    ctx->size = cfg->rx_size;
    ctx->head = 0U;
    ctx->tail = 0U;

    if (cfg->id == USART_ID_1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        ctx->huart.Instance = USART1;
        gpio_init.Alternate = GPIO_AF7_USART1;
        gpio_init.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
        irqn                = USART1_IRQn;
    }
    else
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        ctx->huart.Instance = USART2;
        gpio_init.Alternate = GPIO_AF7_USART2;
        gpio_init.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        irqn                = USART2_IRQn;
    }
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Mode  = GPIO_MODE_AF_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    ctx->huart.Init.BaudRate     = cfg->baudrate;
    ctx->huart.Init.WordLength   = cfg->word_length;
    ctx->huart.Init.StopBits     = cfg->stop_bits;
    ctx->huart.Init.Parity       = cfg->parity;
    ctx->huart.Init.Mode         = cfg->mode;
    ctx->huart.Init.HwFlowCtl    = cfg->hw_flow_ctl;
    ctx->huart.Init.OverSampling = cfg->oversampling;
    (void)HAL_UART_Init(&ctx->huart);

    if ((tx != USART_IO_POLL) || (rx != USART_IO_POLL))
    {
        HAL_NVIC_SetPriority(irqn, cfg->irq_preempt, cfg->irq_sub);
        HAL_NVIC_EnableIRQ(irqn);
    }

    if (tx == USART_IO_DMA)
    {
        usart_dma_tx_init(ctx);
    }

    if (rx == USART_IO_IT)
    {
        (void)HAL_UART_Receive_IT(&ctx->huart, &ctx->rx_byte, 1U);
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
    usart_ctx_t *ctx = &g_uart[id];
    uint16_t     n;

    if ((data == 0) || (len == 0U) || usart_tx_busy(id))
    {
        return false;
    }
    n = (len > 0xFFFFU) ? 0xFFFFU : (uint16_t)len;

    if (ctx->tx == USART_IO_IT)
    {
        return (HAL_UART_Transmit_IT(&ctx->huart, (uint8_t *)data, n) == HAL_OK);
    }
    if (ctx->tx == USART_IO_DMA)
    {
        return (HAL_UART_Transmit_DMA(&ctx->huart, (uint8_t *)data, n) == HAL_OK);
    }
    return (HAL_UART_Transmit(&ctx->huart, (uint8_t *)data, n, HAL_MAX_DELAY) == HAL_OK);
}

uint32_t usart_read(usart_id_t id, uint8_t *data, uint32_t len, uint32_t timeout)
{
    usart_ctx_t *ctx = &g_uart[id];
    uint32_t     n = 0U;
    uint32_t     start = HAL_GetTick();

    while (n < len)
    {
        if (ctx->rx == USART_IO_IT)
        {
            if (ctx->tail != ctx->head)
            {
                data[n++] = ctx->buf[ctx->tail];
                ctx->tail = (uint16_t)((ctx->tail + 1U) % ctx->size);
                start = HAL_GetTick();
                continue;
            }
        }
        else
        {
            if (__HAL_UART_GET_FLAG(&ctx->huart, UART_FLAG_RXNE) != RESET)
            {
                data[n++] = (uint8_t)(ctx->huart.Instance->DR & USART_DATA_MASK);
                start = HAL_GetTick();
                continue;
            }
            if (__HAL_UART_GET_FLAG(&ctx->huart, UART_FLAG_ORE) != RESET)
            {
                __HAL_UART_CLEAR_OREFLAG(&ctx->huart);
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

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart[USART_ID_1].huart);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart[USART_ID_2].huart);
}

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_uart[USART_ID_1].hdma_tx);
}

/**
 * @brief  newlib-nano stdout sink: send one byte over USART1 (blocking).
 */
int __io_putchar(int ch)
{
    uint8_t byte = (uint8_t)ch;

    (void)HAL_UART_Transmit(&g_uart[USART_ID_1].huart, &byte, 1U, USART_TX_TIMEOUT_MS);

    return ch;
}
