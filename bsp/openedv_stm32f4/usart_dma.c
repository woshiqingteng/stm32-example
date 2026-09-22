/**
 * @file    usart_dma.c
 * @brief   USART1 TX over DMA2 Stream7 / channel 4 (facade for apps).
 */

#include "stm32f4xx_hal.h"
#include "usart.h"

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
