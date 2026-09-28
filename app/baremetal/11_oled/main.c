/**
 * @file    main.c
 * @brief   11_oled: SSD1306 8080 parallel OLED demo (12x6 / 8x16 / 24x12) with
 *          serial mirror.
 */

#include <stdio.h>

#include "bsp.h"

#define OLED_TITLE_Y       0U
#define OLED_SUBTITLE_Y    24U
#define OLED_INFO_Y        52U
#define OLED_ASCII_X       36U
#define OLED_CODE_LABEL_X  64U
#define OLED_CODE_X        94U
#define OLED_CODE_DIGIT_COUNT   3U
#define OLED_REFRESH_MS    500U
#define OLED_CHAR_FIRST    ((uint8_t)' ')
#define OLED_CHAR_LAST     ((uint8_t)'~')

int main(void)
{
    char ascii[2];
    uint8_t t = OLED_CHAR_FIRST;

    bsp_init();
    oled_init();

    oled_show_string(0U, OLED_TITLE_Y, "ALIENTEK", OLED_FONT_24X12);
    oled_show_string(0U, OLED_SUBTITLE_Y, "0.96' OLED TEST", OLED_FONT_8X16);
    oled_show_string(0U, OLED_INFO_Y, "ASCII:", OLED_FONT_12X6);
    oled_show_string(OLED_CODE_LABEL_X, OLED_INFO_Y, "CODE:", OLED_FONT_12X6);
    oled_refresh();

    printf(APP_BANNER "\r\n");

    for (;;)
    {
        oled_show_string(OLED_ASCII_X, OLED_INFO_Y, " ", OLED_FONT_12X6);
        oled_show_string(OLED_CODE_X, OLED_INFO_Y, "   ", OLED_FONT_12X6);

        ascii[0] = (char)t;
        ascii[1] = '\0';
        oled_show_string(OLED_ASCII_X, OLED_INFO_Y, ascii, OLED_FONT_12X6);
        oled_show_num(OLED_CODE_X, OLED_INFO_Y, t, OLED_CODE_DIGIT_COUNT, OLED_FONT_12X6);
        oled_refresh();

        printf("ASCII:%c CODE:%u\r\n", (int)t, (unsigned)t);

        t++;
        if (t > OLED_CHAR_LAST)
        {
            t = OLED_CHAR_FIRST;
        }

        delay_ms(OLED_REFRESH_MS);
        led_toggle(LED0);
    }
}
