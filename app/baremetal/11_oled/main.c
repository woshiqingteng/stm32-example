/**
 * @file    main.c
 * @brief   11_oled: SSD1306 8080 parallel OLED demo with serial mirror.
 */

#include <stdio.h>
#include "bsp.h"
#include "oled.h"

int main(void)
{
    char ascii[2];
    uint8_t t = (uint8_t)' ';

    bsp_init();
    oled_init();

    oled_show_string(0, 0, "ALIENTEK", OLED_FONT_8X16);
    oled_show_string(0, 16, "0.96' OLED TEST", OLED_FONT_8X16);
    oled_show_string(0, 48, "ASCII:", OLED_FONT_6X8);
    oled_show_string(64, 48, "CODE:", OLED_FONT_6X8);
    oled_refresh();

    printf("11_oled ready\r\n");

    for (;;)
    {
        oled_show_string(40, 48, " ", OLED_FONT_6X8);
        oled_show_string(88, 48, "   ", OLED_FONT_6X8);

        ascii[0] = (char)t;
        ascii[1] = '\0';
        oled_show_string(40, 48, ascii, OLED_FONT_6X8);
        oled_show_num(88, 48, t, 3, OLED_FONT_6X8);
        oled_refresh();

        printf("ASCII:%c CODE:%u\r\n", (int)t, (unsigned)t);

        t++;
        if (t > (uint8_t)'~')
        {
            t = (uint8_t)' ';
        }

        delay_ms(500);
        led_toggle(LED0);
    }
}
