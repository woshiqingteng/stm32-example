/**
 * @file    main.c
 * @brief   19_dma: USART1 TX over DMA (facade: bsp/usart).
 *
 * KEY0 starts a ~6 KB transfer. The buffer is sent in chunks so progress can be
 * reported over the same USART between DMA transfers; the driver serialises
 * printf with the DMA, so printing mid-transfer would simply block.
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"

#define DMA_TX_BUF_SIZE_BYTE       (6U * 1024U)
#define DMA_TX_CHUNK_BYTE          1024U

#define DMA_TX_LINE_TERM_LEN_BYTE  1U
#define DMA_TX_PROGRESS_SCALE_PERCENT 100U
#define DMA_TX_POLL_DELAY_MS  1U
#define DMA_TX_LOOP_MS  200U

static const char DMA_TX_LINE[] = "USART1 TX DMA: 0123456789\r\n";
static uint8_t    g_tx_buf[DMA_TX_BUF_SIZE_BYTE];

/* Fill with whole lines (the sub-line tail of the buffer stays unused). */
static uint16_t dma_fill_buffer(void)
{
    uint16_t line_len = (uint16_t)(sizeof(DMA_TX_LINE) - DMA_TX_LINE_TERM_LEN_BYTE);
    uint16_t i = 0U;

    while (((uint32_t)i + line_len) <= DMA_TX_BUF_SIZE_BYTE)
    {
        memcpy(&g_tx_buf[i], DMA_TX_LINE, line_len);
        i = (uint16_t)(i + line_len);
    }
    return i;
}

int main(void)
{
    uint16_t len;

    bsp_init();
    printf(APP_BANNER "\r\n");
    {
        usart_cfg_t cfg = { USART_CFG_DEFAULT(USART_ID_1) };

        cfg.tx = USART_IO_DMA;
        usart_init(&cfg);
    }
    len = dma_fill_buffer();

    printf("KEY0: send\r\n");

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            uint16_t offset = 0U;

            printf("DMA TX start: %u bytes\r\n", (unsigned)len);

            while (offset < len)
            {
                uint16_t chunk = (uint16_t)(len - offset);

                if (chunk > DMA_TX_CHUNK_BYTE)
                {
                    chunk = DMA_TX_CHUNK_BYTE;
                }

                if (!usart_write(USART_ID_1, &g_tx_buf[offset], chunk))
                {
                    printf("DMA TX error\r\n");
                    break;
                }
                while (usart_tx_busy(USART_ID_1))
                {
                    led_toggle(LED0);
                    delay_ms(DMA_TX_POLL_DELAY_MS);
                }

                offset = (uint16_t)(offset + chunk);
                printf("progress: %u%%\r\n",
                       (unsigned)(((uint32_t)offset * DMA_TX_PROGRESS_SCALE_PERCENT) / len));
            }

            printf("DMA TX finished\r\n");
        }

        led_toggle(LED0);
        delay_ms(DMA_TX_LOOP_MS);
    }
}
