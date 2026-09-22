/**
 * @file    usart.c
 * @brief   USART1 driver: TX via newlib-nano _write, RX interrupt line reception.
 *
 * MSP content (clock/GPIO/NVIC) is inlined into usart_init() instead of HAL_UART_MspInit().
 * The RX interrupt is handled directly (RXNE register polling) so this driver does
 * not depend on HAL DMA support.
 */

#include "stm32f4xx_hal.h"
#include "usart.h"

#define USART1_TX_PORT    GPIOA
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PORT    GPIOA
#define USART1_RX_PIN     GPIO_PIN_10
#define USART1_GPIO_AF    GPIO_AF7_USART1

#define USART_DATA_MASK             0xFFU
#define USART_RX_BUF_RESERVE        1U
#define USART1_IRQ_PREEMP_PRIORITY  3U
#define USART1_IRQ_SUB_PRIORITY     3U

#define USART_TX_TIMEOUT_MS         1000U

UART_HandleTypeDef g_uart1_handle;

static usart_rx_byte_cb_t g_rx_byte_cb;

static uint8_t          g_rx_buf[USART_REC_LEN];
static uint16_t         g_rx_len;
static usart_rx_state_t g_rx_state;

static void usart_rx_byte(uint8_t byte)
{
    if (g_rx_byte_cb != 0)
    {
        g_rx_byte_cb(byte);
    }

    /* CR and LF are both end-of-line markers, so the terminator handling is
     * symmetric; a repeated terminator (for example LF after CR) is ignored
     * while the completed line waits to be consumed. */
    if ((byte == '\r') || (byte == '\n'))
    {
        if (g_rx_state == USART_RX_OVERFLOW)
        {
            g_rx_len   = 0U;
            g_rx_state = USART_RX_IDLE;
        }
        else if (g_rx_state != USART_RX_READY)
        {
            g_rx_state = USART_RX_READY;
        }

        return;
    }

    if ((g_rx_state == USART_RX_READY) || (g_rx_state == USART_RX_OVERFLOW))
    {
        return;
    }

    if (g_rx_len < (USART_REC_LEN - USART_RX_BUF_RESERVE))
    {
        g_rx_buf[g_rx_len++] = byte;
        g_rx_state = USART_RX_RECEIVING;
    }
    else
    {
        g_rx_len   = 0U;
        g_rx_state = USART_RX_OVERFLOW;
    }
}

void usart_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: clock + GPIO + NVIC ---- */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = USART1_GPIO_AF;

    gpio_init.Pin = USART1_TX_PIN;
    HAL_GPIO_Init(USART1_TX_PORT, &gpio_init);

    gpio_init.Pin = USART1_RX_PIN;
    HAL_GPIO_Init(USART1_RX_PORT, &gpio_init);
    /* ---- MSP end ---- */

    g_uart1_handle.Instance          = USART1;
    g_uart1_handle.Init.BaudRate     = baudrate;
    g_uart1_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    g_uart1_handle.Init.StopBits     = UART_STOPBITS_1;
    g_uart1_handle.Init.Parity       = UART_PARITY_NONE;
    g_uart1_handle.Init.Mode         = UART_MODE_TX_RX;
    g_uart1_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    g_uart1_handle.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&g_uart1_handle);

    __HAL_UART_ENABLE_IT(&g_uart1_handle, UART_IT_RXNE);

    HAL_NVIC_SetPriority(USART1_IRQn, USART1_IRQ_PREEMP_PRIORITY, USART1_IRQ_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void usart_register_rx_byte_hook(usart_rx_byte_cb_t cb)
{
    g_rx_byte_cb = cb;
}

void USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&g_uart1_handle, UART_FLAG_RXNE) != RESET)
    {
        usart_rx_byte((uint8_t)(USART1->DR & USART_DATA_MASK));
    }

    if (__HAL_UART_GET_FLAG(&g_uart1_handle, UART_FLAG_ORE) != RESET)
    {
        __HAL_UART_CLEAR_OREFLAG(&g_uart1_handle);
    }
}

usart_rx_state_t usart_rx_state(void)
{
    return g_rx_state;
}

uint16_t usart_rx_len(void)
{
    return g_rx_len;
}

const uint8_t *usart_rx_buf(void)
{
    return g_rx_buf;
}

void usart_rx_clear(void)
{
    g_rx_len   = 0;
    g_rx_state = USART_RX_IDLE;
}

/**
 * @brief  newlib-nano stdout hook: send a buffer over USART1.
 */
int _write(int file, char *ptr, int len)
{
    (void)file;

    if (len <= 0)
    {
        return 0;
    }

    (void)HAL_UART_Transmit(&g_uart1_handle, (uint8_t *)ptr, (uint16_t)len, USART_TX_TIMEOUT_MS);

    return len;
}

/* ---- USART1 TX over DMA2 Stream7 / channel 4 ---- */

#define USART_DMA_TX_STREAM   DMA2_Stream7
#define USART_DMA_TX_CHANNEL  DMA_CHANNEL_4
#define USART_DMA_TX_IRQn     DMA2_Stream7_IRQn
#define USART_DMA_TX_PREEMP   3U
#define USART_DMA_TX_SUB      3U

static DMA_HandleTypeDef g_uart_tx_dma;

void usart_tx_dma_init(void)
{
    /* ---- MSP begin: DMA2 clock + NVIC ---- */
    __HAL_RCC_DMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(USART_DMA_TX_IRQn, USART_DMA_TX_PREEMP, USART_DMA_TX_SUB);
    HAL_NVIC_EnableIRQ(USART_DMA_TX_IRQn);
    /* ---- MSP end ---- */

    __HAL_LINKDMA(&g_uart1_handle, hdmatx, g_uart_tx_dma);

    g_uart_tx_dma.Instance                 = USART_DMA_TX_STREAM;
    g_uart_tx_dma.Init.Channel             = USART_DMA_TX_CHANNEL;
    g_uart_tx_dma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    g_uart_tx_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_uart_tx_dma.Init.MemInc              = DMA_MINC_ENABLE;
    g_uart_tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    g_uart_tx_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    g_uart_tx_dma.Init.Mode                = DMA_NORMAL;
    g_uart_tx_dma.Init.Priority            = DMA_PRIORITY_MEDIUM;
    g_uart_tx_dma.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    g_uart_tx_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    g_uart_tx_dma.Init.MemBurst            = DMA_MBURST_SINGLE;
    g_uart_tx_dma.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    HAL_DMA_DeInit(&g_uart_tx_dma);
    (void)HAL_DMA_Init(&g_uart_tx_dma);
}

bool usart_tx_dma_busy(void)
{
    return (g_uart1_handle.gState != HAL_UART_STATE_READY);
}

bool usart_tx_dma(const uint8_t *data, uint16_t len)
{
    if (usart_tx_dma_busy())
    {
        return false;
    }

    return (HAL_UART_Transmit_DMA(&g_uart1_handle, (uint8_t *)data, len) == HAL_OK);
}

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_uart_tx_dma);
}
