/**
 * @file    main.c
 * @brief   31_remote: NEC infrared remote test. The decoded key code and repeat
 *          count are printed and shown on the RGB panel.
 */

#include <stdio.h>
#include "bsp.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U

static const char *remote_symbol(uint8_t key)
{
    switch (key)
    {
        case 69U: return "POWER";
        case 70U: return "UP";
        case 64U: return "PLAY";
        case 71U: return "ALIENTEK";
        case 67U: return "RIGHT";
        case 68U: return "LEFT";
        case 7U:  return "VOL-";
        case 21U: return "DOWN";
        case 9U:  return "VOL+";
        case 22U: return "1";
        case 25U: return "2";
        case 13U: return "3";
        case 12U: return "4";
        case 24U: return "5";
        case 94U: return "6";
        case 8U:  return "7";
        case 28U: return "8";
        case 90U: return "9";
        case 66U: return "0";
        case 74U: return "DELETE";
        default:  return "UNKNOWN";
    }
}

int main(void)
{
    uint8_t key;
    uint8_t led_tick = 0U;
    char    line[48];

    bsp_init();
    sdram_init();
    lcd_init();
    remote_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "REMOTE TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(TEXT_X, 90U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "KEYVAL:", BLUE);
    lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "KEYCNT:", BLUE);
    lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "SYMBOL:", BLUE);

    printf("31_remote ready\r\n");

    for (;;)
    {
        key = remote_scan();

        if (key != 0U)
        {
            lcd_show_xnum(TEXT_X + 60U, 90U, key, 3U, LCD_FONT_SIZE_16,
                          LCD_TEXT_BG_OVERWRITE, BLUE);
            lcd_show_xnum(TEXT_X + 60U, 110U, g_remote_cnt, 3U, LCD_FONT_SIZE_16,
                          LCD_TEXT_BG_OVERWRITE, BLUE);
            lcd_show_string(TEXT_X + 60U, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                            remote_symbol(key), BLUE);

            sprintf(line, "KEY: %u CNT: %u SYM: %s", key, g_remote_cnt, remote_symbol(key));
            printf("%s\r\n", line);

            delay_ms(100U);
        }
        else
        {
            delay_ms(10U);
        }

        led_tick++;
        if (led_tick >= 50U)
        {
            led_tick = 0U;
            led_toggle(LED0);
        }
    }
}
