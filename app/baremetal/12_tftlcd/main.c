/**
 * @file    main.c
 * @brief   12_tftlcd: SSD1963 MCU-screen colour cycle demo.
 */

#include <stdio.h>
#include "bsp.h"
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
    lcd_init();

    sprintf(lcd_id, "LCD ID:%04X", (unsigned int)lcd_get_id());
    printf("12_tftlcd ready, %s\r\n", lcd_id);

    for (;;)
    {
        lcd_clear(colors[x]);
        lcd_show_string(10, 40, "STM32", 16, RED);
        lcd_show_string(10, 70, "TFTLCD TEST", 16, RED);
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
