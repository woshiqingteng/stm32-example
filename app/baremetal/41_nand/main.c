/**
 * @file    main.c
 * @brief   41_nand: NAND FLASH test. The device ID is read, one block is
 *          erased and a page is written, read back and verified. Results are
 *          shown on the RGB panel and USART1.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "nand.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U
#define TEST_BLOCK      0U
#define TEST_PAGE       0U
#define TEST_LEN        NAND_ECC_SECTOR_SIZE
#define BLINK_PERIOD_MS 500U

static uint8_t g_wbuf[TEST_LEN];
static uint8_t g_rbuf[TEST_LEN];

int main(void)
{
    uint8_t  res;
    uint32_t errors = 0U;
    uint32_t i;
    char     line[64];

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "NAND TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    printf("41_nand ready\r\n");

    res = nand_init();

    if (res != 0U)
    {
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "NAND Error!", RED);
        printf("NAND init failed\r\n");
    }
    else
    {
        nand_info_t info;

        nand_get_info(&info);

        sprintf(line, "ID:%08lX Size:%luMB", (unsigned long)info.id, (unsigned long)info.size_mb);
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
        printf("%s\r\n", line);

        (void)nand_eraseblock(TEST_BLOCK);

        for (i = 0U; i < TEST_LEN; i++)
        {
            g_wbuf[i] = (uint8_t)(i * 5U + 0x5AU);
        }

        res = nand_writepage(TEST_PAGE, 0U, g_wbuf, TEST_LEN);
        if (res != 0U)
        {
            errors = TEST_LEN;
        }
        else
        {
            res = nand_readpage(TEST_PAGE, 0U, g_rbuf, TEST_LEN);

            for (i = 0U; i < TEST_LEN; i++)
            {
                if (g_rbuf[i] != g_wbuf[i])
                {
                    errors++;
                }
            }
        }

        sprintf(line, "Page %u: %s (%lu err)", (unsigned)TEST_PAGE,
                (errors == 0U) ? "OK" : "FAIL", (unsigned long)errors);
        lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line,
                        (errors == 0U) ? BLUE : RED);
        printf("%s\r\n", line);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
