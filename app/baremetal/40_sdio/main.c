/**
 * @file    main.c
 * @brief   40_sdio: external SRAM test plus SD card over SDIO. The card type
 *          and capacity are printed, a block is written and read back and the
 *          result is shown on the RGB panel and USART1.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "sdio.h"
#include "sram.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U
#define SRAM_TEST_LEN   512U
#define SD_TEST_SECTOR  1000U
#define SD_TEST_COUNT   1U
#define SD_BLOCK_LEN    512U
#define BLINK_PERIOD_MS 500U

static uint8_t g_wbuf[SD_BLOCK_LEN];
static uint8_t g_rbuf[SD_BLOCK_LEN];

int main(void)
{
    HAL_SD_CardInfoTypeDef info;
    uint32_t errors = 0U;
    uint32_t i;
    uint32_t sram_errors;
    char     line[64];

    bsp_init();
    sdram_init();
    sram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "SD / SRAM TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    printf("40_sdio ready\r\n");

    sram_errors = sram_test(0U, SRAM_TEST_LEN);
    printf("SRAM 0x%08lX: %lu error(s)\r\n", (unsigned long)SRAM_BASE_ADDR,
           (unsigned long)sram_errors);

    if (sdio_init() != 0U)
    {
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "SD Card Error!", RED);
        printf("SD init failed\r\n");
    }
    else
    {
        get_sd_card_info(&info);

        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "SD Card OK", BLUE);
        sprintf(line, "Type:%lu Cap:%lu MB Blk:%lu",
                (unsigned long)info.CardType,
                (unsigned long)SD_TOTAL_SIZE_MB(&g_sdcard_handle),
                (unsigned long)info.LogBlockSize);
        lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
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
        lcd_show_string(TEXT_X, 150U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line,
                        (errors == 0U) ? BLUE : RED);
        printf("%s\r\n", line);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
