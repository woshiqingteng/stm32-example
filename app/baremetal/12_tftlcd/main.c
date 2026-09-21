/**
 * @file    main.c
 * @brief   12_tftlcd: screen colour cycle demo.
 *
 * NOTE: the MCU screen (SSD1963/FMC) driver has been removed; this former
 * MCU-screen experiment now drives the RGB (LTDC) screen via the same lcd API
 * as 14_ltdc.
 */

#include <stdio.h>
#include "bsp.h"
#include "sdram.h"
#include "lcd.h"

#define LCD_COLOR_COUNT 12U

int main(void)
{
    static const uint16_t colors[LCD_COLOR_COUNT] =
    {
        WHITE, BLACK, BLUE, RED, MAGENTA, GREEN,
        CYAN, YELLOW, BRRED, GRAY, LGRAY, BROWN
    };
    char lcd_id[16];
    uint8_t x = 0;

    bsp_init();
    sdram_init();
    lcd_init();

    g_point_color = RED;
    sprintf(lcd_id, "LCD ID:%04X", (unsigned int)lcddev.id);
    printf("12_tftlcd ready (RGB screen), %s\r\n", lcd_id);

    for (;;)
    {
        lcd_clear(colors[x]);
        lcd_show_string(10, 40, 240, 32, 32, "STM32", RED);
        lcd_show_string(10, 80, 240, 24, 24, "LTDC TEST", RED);
        lcd_show_string(10, 110, 240, 16, 16, "ATOM@ALIENTEK", RED);
        lcd_show_string(10, 130, 240, 16, 16, lcd_id, RED);

        printf("color index %u\r\n", (unsigned)x);

        x++;
        if (x >= LCD_COLOR_COUNT)
        {
            x = 0;
        }

        led_toggle(LED0);
        delay_ms(1000);
    }
}
