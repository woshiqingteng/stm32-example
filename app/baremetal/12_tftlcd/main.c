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
#define DEMO_TEXT_X     10U
#define DEMO_TEXT_WIDTH 240U
#define DEMO_ID_Y       130U
#define DEMO_ID_SIZE    16U
#define DEMO_REFRESH_MS 1000U

typedef struct
{
    uint16_t    y;
    uint8_t     size;
    const char *text;
} demo_line_t;

static const demo_line_t g_demo_lines[] =
{
    {  40U, 32U, "STM32"         },
    {  80U, 24U, "LTDC TEST"     },
    { 110U, 16U, "ATOM@ALIENTEK" }
};

#define DEMO_LINE_COUNT (sizeof(g_demo_lines) / sizeof(g_demo_lines[0]))

int main(void)
{
    static const uint16_t colors[LCD_COLOR_COUNT] =
    {
        WHITE, BLACK, BLUE, RED, MAGENTA, GREEN,
        CYAN, YELLOW, BRRED, GRAY, LGRAY, BROWN
    };
    char lcd_id[16];
    uint8_t x = 0;
    uint8_t i;

    bsp_init();
    sdram_init();
    lcd_init();

    g_point_color = RED;
    sprintf(lcd_id, "LCD ID:%04X", (unsigned int)lcd_get_id());
    printf("12_tftlcd ready (RGB screen), %s\r\n", lcd_id);

    for (;;)
    {
        lcd_clear(colors[x]);

        for (i = 0U; i < (uint8_t)DEMO_LINE_COUNT; i++)
        {
            lcd_show_string(DEMO_TEXT_X, g_demo_lines[i].y, DEMO_TEXT_WIDTH,
                            g_demo_lines[i].size, g_demo_lines[i].size,
                            g_demo_lines[i].text, RED);
        }

        lcd_show_string(DEMO_TEXT_X, DEMO_ID_Y, DEMO_TEXT_WIDTH, DEMO_ID_SIZE, DEMO_ID_SIZE, lcd_id, RED);

        printf("color index %u\r\n", (unsigned)x);

        x++;
        if (x >= LCD_COLOR_COUNT)
        {
            x = 0;
        }

        led_toggle(LED0);
        delay_ms(DEMO_REFRESH_MS);
    }
}
