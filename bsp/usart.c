/**
 * @file    usart.c
 * @brief   USART1 driver: TX via newlib-nano _write, RX interrupt line reception.
 *
 * MSP content (clock/GPIO/NVIC) is inlined into usart_init() instead of HAL_UART_MspInit().
 */

#include "stm32f4xx_hal.h"
#include "usart.h"

#define USART1_TX_PORT    GPIOA
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PORT    GPIOA
#define USART1_RX_PIN     GPIO_PIN_10
#define USART1_GPIO_AF    GPIO_AF7_USART1

#define USART1_DMA_STREAM   DMA2_Stream7
#define USART1_DMA_CHANNEL  DMA_CHANNEL_4
#define USART1_DMA_IRQn     DMA2_Stream7_IRQn

UART_HandleTypeDef g_uart1_handle;

static DMA_HandleTypeDef  g_uart1_tx_dma;
static usart_rx_byte_cb_t g_rx_byte_cb;

static uint8_t          g_rx_byte;
static uint8_t          g_rx_buf[USART_REC_LEN];
static uint16_t         g_rx_len;
static usart_rx_state_t g_rx_state;

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

    HAL_NVIC_SetPriority(USART1_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_UART_Receive_IT(&g_uart1_handle, &g_rx_byte, 1);
}

void usart_register_rx_byte_hook(usart_rx_byte_cb_t cb)
{
    g_rx_byte_cb = cb;
}

void usart_dma_tx_init(void)
{
    /* ---- MSP begin: DMA2 clock + NVIC ---- */
    __HAL_RCC_DMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(USART1_DMA_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(USART1_DMA_IRQn);
    /* ---- MSP end ---- */

    __HAL_LINKDMA(&g_uart1_handle, hdmatx, g_uart1_tx_dma);

    g_uart1_tx_dma.Instance                 = USART1_DMA_STREAM;
    g_uart1_tx_dma.Init.Channel             = USART1_DMA_CHANNEL;
    g_uart1_tx_dma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    g_uart1_tx_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_uart1_tx_dma.Init.MemInc              = DMA_MINC_ENABLE;
    g_uart1_tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    g_uart1_tx_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    g_uart1_tx_dma.Init.Mode                = DMA_NORMAL;
    g_uart1_tx_dma.Init.Priority            = DMA_PRIORITY_MEDIUM;
    g_uart1_tx_dma.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    g_uart1_tx_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    g_uart1_tx_dma.Init.MemBurst            = DMA_MBURST_SINGLE;
    g_uart1_tx_dma.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    HAL_DMA_DeInit(&g_uart1_tx_dma);
    (void)HAL_DMA_Init(&g_uart1_tx_dma);
}

void usart_dma_tx(const uint8_t *data, uint16_t len)
{
    if (g_uart1_handle.gState != HAL_UART_STATE_READY)
    {
        return;
    }
    (void)HAL_UART_Transmit_DMA(&g_uart1_handle, data, len);
}

uint8_t usart_dma_tx_busy(void)
{
    return (g_uart1_handle.gState != HAL_UART_STATE_READY) ? 1U : 0U;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (g_rx_byte_cb != 0)
        {
            g_rx_byte_cb(g_rx_byte);
        }

        if ((g_rx_state == USART_RX_CR) && (g_rx_byte == '\n'))
        {
            g_rx_state = USART_RX_READY;
        }
        else if (g_rx_state != USART_RX_READY)
        {
            if (g_rx_byte == '\r')
            {
                g_rx_state = USART_RX_CR;
            }
            else
            {
                if (g_rx_state == USART_RX_CR)
                {
                    g_rx_state = USART_RX_IDLE;
                }
                if (g_rx_len < (USART_REC_LEN - 1U))
                {
                    g_rx_buf[g_rx_len++] = g_rx_byte;
                }
                else
                {
                    g_rx_len = 0;
                }
            }
        }

        HAL_UART_Receive_IT(&g_uart1_handle, &g_rx_byte, 1);
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart1_handle);
}

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_uart1_tx_dma);
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
    int i;

    (void)file;

    for (i = 0; i < len; i++)
    {
        while ((USART1->SR & USART_SR_TXE) == 0U)
        {
        }
        USART1->DR = (uint16_t)(uint8_t)ptr[i];
    }

    while ((USART1->SR & USART_SR_TC) == 0U)
    {
    }

    return len;
}
