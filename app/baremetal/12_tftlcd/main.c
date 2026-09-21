/**
 * @file    main.c
 * @brief   12_tftlcd: screen colour cycle demo.
 *
 * NOTE: the MCU screen (SSD1963) driver has been removed from this project, so
 * this former MCU-screen experiment now drives the RGB screen (LTDC) directly.
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
    char lcd_id[24];
    uint8_t x = 0;

    bsp_init();
    sdram_init();
    lcd_init();

    sprintf(lcd_id, "LCD ID:%04X", (unsigned int)lcd_get_id());
    printf("12_tftlcd ready (MCU driver removed, RGB screen), %s\r\n", lcd_id);

    for (;;)
    {
        lcd_clear(colors[x]);
        lcd_show_string(10, 40, "STM32", 16, RED);
        lcd_show_string(10, 70, "RGB SCREEN", 16, RED);
        lcd_show_string(10, 100, "ATOM@ALIENTEK", 16, RED);
        lcd_show_string(10, 130, lcd_id, 16, RED);
        lcd_show_num(10, 160, x, 2, 16, RED);

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
