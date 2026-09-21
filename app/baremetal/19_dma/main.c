/**
 * @file    main.c
 * @brief   19_dma: USART1 TX over DMA2 Stream7 / channel 4.
 *
 * KEY0 starts a ~6 KB transfer. The buffer is sent in chunks so that progress
 * can be reported over the same USART between DMA transfers (printing while the
 * USART is driven by DMA would corrupt the byte stream).
 */

#include <stdio.h>
#include "bsp.h"

#define DMA_TX_BUF_SIZE (6U * 1024U)
#define DMA_TX_CHUNK    1024U

#define DMA_TX_STREAM   DMA2_Stream7
#define DMA_TX_CHANNEL  DMA_CHANNEL_4
#define DMA_TX_IRQn     DMA2_Stream7_IRQn

static const char DMA_TX_LINE[] = "STM32F429 USART1 TX DMA demo - 0123456789\r\n";
static uint8_t    g_tx_buf[DMA_TX_BUF_SIZE];

static DMA_HandleTypeDef g_dma_tx;

static void dma_tx_init(void)
{
    /* ---- MSP begin: DMA2 clock + NVIC ---- */
    __HAL_RCC_DMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA_TX_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(DMA_TX_IRQn);
    /* ---- MSP end ---- */

    __HAL_LINKDMA(&g_uart1_handle, hdmatx, g_dma_tx);

    g_dma_tx.Instance                 = DMA_TX_STREAM;
    g_dma_tx.Init.Channel             = DMA_TX_CHANNEL;
    g_dma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    g_dma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_dma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    g_dma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    g_dma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    g_dma_tx.Init.Mode                = DMA_NORMAL;
    g_dma_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    g_dma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    g_dma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    g_dma_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
    g_dma_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    HAL_DMA_DeInit(&g_dma_tx);
    (void)HAL_DMA_Init(&g_dma_tx);
}

static void dma_tx(const uint8_t *data, uint16_t len)
{
    if (g_uart1_handle.gState != HAL_UART_STATE_READY)
    {
        return;
    }
    (void)HAL_UART_Transmit_DMA(&g_uart1_handle, data, len);
}

static uint8_t dma_tx_busy(void)
{
    return (g_uart1_handle.gState != HAL_UART_STATE_READY) ? 1U : 0U;
}

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_tx);
}

static uint16_t dma_fill_buffer(void)
{
    uint16_t line_len = (uint16_t)(sizeof(DMA_TX_LINE) - 1U);
    uint16_t i = 0U;

    while (((uint32_t)i + line_len) <= DMA_TX_BUF_SIZE)
    {
        uint16_t k;

        for (k = 0U; k < line_len; k++)
        {
            g_tx_buf[i + k] = (uint8_t)DMA_TX_LINE[k];
        }
        i = (uint16_t)(i + line_len);
    }
    return i;
}

int main(void)
{
    uint16_t len;

    bsp_init();
    dma_tx_init();
    len = dma_fill_buffer();

    printf("19_dma ready: %u bytes buffered, press KEY0 to send\r\n", (unsigned)len);

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            uint16_t offset = 0U;

            printf("DMA TX start: %u bytes\r\n", (unsigned)len);

            while (offset < len)
            {
                uint16_t chunk = (uint16_t)(len - offset);

                if (chunk > DMA_TX_CHUNK)
                {
                    chunk = DMA_TX_CHUNK;
                }

                dma_tx(&g_tx_buf[offset], chunk);
                while (dma_tx_busy())
                {
                    led_toggle(LED0);
                    delay_ms(1);
                }

                offset = (uint16_t)(offset + chunk);
                printf("progress: %u%%\r\n", (unsigned)(((uint32_t)offset * 100U) / len));
            }

            printf("DMA TX finished\r\n");
        }

        led_toggle(LED0);
        delay_ms(100);
    }
}
