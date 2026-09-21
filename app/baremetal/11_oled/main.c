/**
 * @file    main.c
 * @brief   11_oled: SSD1306 8080 parallel OLED demo with serial mirror.
 */

#include <stdio.h>
#include "bsp.h"
#include "oled.h"

#define OLED_DEMO_TITLE_Y      0U
#define OLED_DEMO_SUBTITLE_Y   16U
#define OLED_DEMO_INFO_Y       48U
#define OLED_DEMO_ASCII_X      40U
#define OLED_DEMO_CODE_LABEL_X 64U
#define OLED_DEMO_CODE_X       88U
#define OLED_DEMO_CODE_DIGITS  3U
#define OLED_DEMO_REFRESH_MS   500U
#define OLED_CHAR_FIRST        ((uint8_t)' ')
#define OLED_CHAR_LAST         ((uint8_t)'~')

int main(void)
{
    char ascii[2];
    uint8_t t = OLED_CHAR_FIRST;

    bsp_init();
    oled_init();

    oled_show_string(0U, OLED_DEMO_TITLE_Y, "ALIENTEK", OLED_FONT_8X16);
    oled_show_string(0U, OLED_DEMO_SUBTITLE_Y, "0.96' OLED TEST", OLED_FONT_8X16);
    oled_show_string(0U, OLED_DEMO_INFO_Y, "ASCII:", OLED_FONT_6X8);
    oled_show_string(OLED_DEMO_CODE_LABEL_X, OLED_DEMO_INFO_Y, "CODE:", OLED_FONT_6X8);
    oled_refresh();

    printf("11_oled ready\r\n");

    for (;;)
    {
        oled_show_string(OLED_DEMO_ASCII_X, OLED_DEMO_INFO_Y, " ", OLED_FONT_6X8);
        oled_show_string(OLED_DEMO_CODE_X, OLED_DEMO_INFO_Y, "   ", OLED_FONT_6X8);

        ascii[0] = (char)t;
        ascii[1] = '\0';
        oled_show_string(OLED_DEMO_ASCII_X, OLED_DEMO_INFO_Y, ascii, OLED_FONT_6X8);
        oled_show_num(OLED_DEMO_CODE_X, OLED_DEMO_INFO_Y, t, OLED_DEMO_CODE_DIGITS,
                      OLED_FONT_6X8);
        oled_refresh();

        printf("ASCII:%c CODE:%u\r\n", (int)t, (unsigned)t);

        t++;
        if (t > OLED_CHAR_LAST)
        {
            t = OLED_CHAR_FIRST;
        }

        delay_ms(OLED_DEMO_REFRESH_MS);
        led_toggle(LED0);
    }
}
