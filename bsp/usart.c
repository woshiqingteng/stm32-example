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

    if ((g_rx_state == USART_RX_CR) && (byte == '\n'))
    {
        g_rx_state = USART_RX_READY;
    }
    else if (g_rx_state != USART_RX_READY)
    {
        if (byte == '\r')
        {
            g_rx_state = USART_RX_CR;
        }
        else
        {
            if (g_rx_state == USART_RX_CR)
            {
                g_rx_state = USART_RX_IDLE;
            }
            if (g_rx_len < (USART_REC_LEN - USART_RX_BUF_RESERVE))
            {
                g_rx_buf[g_rx_len++] = byte;
            }
            else
            {
                g_rx_len = 0;
            }
        }
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
