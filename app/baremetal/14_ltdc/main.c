/**
 * @file    main.c
 * @brief   14_ltdc: RGB LTDC colour cycle demo with an SDRAM frame buffer.
 */

#include <stdio.h>
#include "bsp.h"
#include "sdram.h"
#include "ltdc.h"

#define LTDC_COLOR_COUNT 12U

int main(void)
{
    static const uint16_t colors[LTDC_COLOR_COUNT] =
    {
        WHITE, BLACK, BLUE, RED, MAGENTA, GREEN,
        CYAN, YELLOW, BRRED, GRAY, LGRAY, BROWN
    };
    uint8_t x = 0;

    bsp_init();
    sdram_init();
    ltdc_init();

    printf("14_ltdc ready\r\n");

    for (;;)
    {
        ltdc_clear(colors[x]);
        ltdc_show_string(10, 40, "STM32", 16, RED);
        ltdc_show_string(10, 70, "LTDC TEST", 16, RED);
        ltdc_show_string(10, 100, "ATOM@ALIENTEK", 16, RED);
        ltdc_show_string(10, 130, "LCD ID:4384", 16, RED);
        ltdc_show_num(10, 160, x, 2, 16, RED);

        printf("color index %u\r\n", (unsigned)x);

        x++;
        if (x >= LTDC_COLOR_COUNT)
        {
            x = 0;
        }

        led_toggle(LED0);
        delay_ms(1000);
    }
}
