/**
 * @file    main.c
 * @brief   40_sdio: external SRAM test plus SD card over SDIO. The card type
 *          and capacity are printed on USART1; a block is written and read
 *          back and the result is reported on USART1.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "sdio.h"
#include "sram.h"

#define SRAM_TEST_LEN   512U
#define SD_TEST_SECTOR  1000U
#define SD_TEST_COUNT   1U
#define SD_BLOCK_LEN    512U
#define BLINK_PERIOD_MS 500U

static uint8_t g_wbuf[SD_BLOCK_LEN];
static uint8_t g_rbuf[SD_BLOCK_LEN];

int main(void)
{
    sd_card_info_t info;
    uint32_t errors = 0U;
    uint32_t i;
    uint32_t sram_errors;
    char     line[64];

    bsp_init();
    sram_init();

    printf("40_sdio ready\r\n");

    sram_errors = sram_test(0U, SRAM_TEST_LEN);
    printf("SRAM 0x%08lX: %lu error(s)\r\n", (unsigned long)SRAM_BASE_ADDR,
           (unsigned long)sram_errors);

    if (sdio_init() != 0U)
    {
        printf("SD init failed\r\n");
    }
    else
    {
        sdio_get_card_info(&info);

        printf("SD Card OK\r\n");
        sprintf(line, "Type:%lu Cap:%lu MB Blk:%lu",
                (unsigned long)info.card_type,
                (unsigned long)info.total_size_mb,
                (unsigned long)info.block_size);
        printf("%s\r\n", line);

        for (i = 0U; i < SD_BLOCK_LEN; i++)
        {
            g_wbuf[i] = (uint8_t)(i * 3U + 1U);
        }

        if (sd_write_disk(g_wbuf, SD_TEST_SECTOR, SD_TEST_COUNT) == 0U)
        {
            (void)sd_read_disk(g_rbuf, SD_TEST_SECTOR, SD_TEST_COUNT);

            for (i = 0U; i < SD_BLOCK_LEN; i++)
            {
                if (g_rbuf[i] != g_wbuf[i])
                {
                    errors++;
                }
            }
        }
        else
        {
            errors = SD_BLOCK_LEN;
        }

        sprintf(line, "SD R/W @%u: %s (%lu err)", (unsigned)SD_TEST_SECTOR,
                (errors == 0U) ? "OK" : "FAIL", (unsigned long)errors);
        printf("%s\r\n", line);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
