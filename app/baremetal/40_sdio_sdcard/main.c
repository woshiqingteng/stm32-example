/**
 * @file    main.c
 * @brief   40_sdio_sdcard: SD card over SDIO. The card type and capacity are
 *          printed on USART1; a block is written and read back and the result
 *          is reported on USART1.
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "sdio.h"

#define SD_TEST_SECTOR  1000U
#define SD_TEST_COUNT   1U
#define SD_BLOCK_LEN_BYTE    512U
#define BLINK_PERIOD_MS 500U

static uint8_t g_wbuf[SD_BLOCK_LEN_BYTE];
static uint8_t g_rbuf[SD_BLOCK_LEN_BYTE];

int main(void)
{
    sd_card_info_t info;
    uint32_t errors = 0U;
    uint32_t i;

    bsp_init();

    printf(APP_BANNER "\r\n");

    if (sdio_init() != 0U)
    {
        printf("SD init failed\r\n");
    }
    else
    {
        sdio_get_card_info(&info);

        printf("SD Card OK\r\n");
        printf("Type:%lu Cap:%lu MB Blk:%lu\r\n",
               (unsigned long)info.card_type,
               (unsigned long)info.total_size_mb,
               (unsigned long)info.block_size);

        for (i = 0U; i < SD_BLOCK_LEN_BYTE; i++)
        {
            g_wbuf[i] = (uint8_t)(i * 3U + 1U);
        }

        if (sd_write_disk(g_wbuf, SD_TEST_SECTOR, SD_TEST_COUNT) == 0U)
        {
            (void)sd_read_disk(g_rbuf, SD_TEST_SECTOR, SD_TEST_COUNT);

            for (i = 0U; i < SD_BLOCK_LEN_BYTE; i++)
            {
                if (g_rbuf[i] != g_wbuf[i])
                {
                    errors++;
                }
            }
        }
        else
        {
            errors = SD_BLOCK_LEN_BYTE;
        }

        printf("SD R/W @%u: %s (%lu err)\r\n", (unsigned)SD_TEST_SECTOR,
               (errors == 0U) ? "OK" : "FAIL", (unsigned long)errors);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
