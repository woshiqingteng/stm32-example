/**
 * @file    main.c
 * @brief   12_tftlcd: screen colour cycle demo.
 *
 * NOTE: the MCU screen (SSD1963) driver and the lcd forwarding layer have been
 * removed; this former MCU-screen experiment drives the RGB (LTDC) screen
 * directly through the ltdc driver.
 */

#include <stdio.h>
#include "bsp.h"
#include "sdram.h"
#include "ltdc.h"

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
    ltdc_init();

    sprintf(lcd_id, "LCD ID:%04X", (unsigned int)ltdc_panelid_read());
    printf("12_tftlcd ready (RGB screen), %s\r\n", lcd_id);

    for (;;)
    {
        ltdc_clear(colors[x]);
        ltdc_show_string(10, 40, "STM32", 16, RED);
        ltdc_show_string(10, 70, "RGB SCREEN", 16, RED);
        ltdc_show_string(10, 100, "ATOM@ALIENTEK", 16, RED);
        ltdc_show_string(10, 130, lcd_id, 16, RED);
        ltdc_show_num(10, 160, x, 2, 16, RED);

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
