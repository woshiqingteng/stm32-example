/**
 * @file    usart.c
 * @brief   USART1 driver with newlib-nano _write redirection.
 *
 * MSP content (clock/GPIO) is inlined into usart_init() instead of implementing HAL_UART_MspInit().
 */

#include "stm32f4xx_hal.h"
#include "usart.h"

#define USART1_TX_PORT    GPIOA
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PORT    GPIOA
#define USART1_RX_PIN     GPIO_PIN_10
#define USART1_GPIO_AF    GPIO_AF7_USART1

UART_HandleTypeDef g_uart1_handle;

void usart_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: clock + GPIO ---- */
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
}

/**
 * @brief  newlib-nano stdout hook: send a buffer over USART1.
 * @param  file Unused file descriptor.
 * @param  ptr  Data buffer.
 * @param  len  Number of bytes.
 * @return Number of bytes written.
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
