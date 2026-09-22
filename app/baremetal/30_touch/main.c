/**
 * @file    main.c
 * @brief   30_touch: GT9147 capacitive touch test. Touch points and the trail
 *          between consecutive samples are drawn on the RGB panel; coordinates
 *          are reported over USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define TOUCH_POINT_SIZE  2U
#define TOUCH_SAMPLE_MS   5U
#define TOUCH_LED_DIV     40U
#define TOUCH_TEXT_X      10U
#define TOUCH_TEXT_WIDTH  400U

static void touch_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t dx;
    uint8_t dy;

    for (dy = 0U; dy < TOUCH_POINT_SIZE; dy++)
    {
        for (dx = 0U; dx < TOUCH_POINT_SIZE; dx++)
        {
            lcd_draw_point((uint16_t)(x + dx), (uint16_t)(y + dy), color);
        }
    }
}

static void touch_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int32_t dx = (int32_t)x2 - (int32_t)x1;
    int32_t dy = (int32_t)y2 - (int32_t)y1;
    int32_t steps;
    int32_t i;

    steps = (dx < 0) ? -dx : dx;

    if (((dy < 0) ? -dy : dy) > steps)
    {
        steps = (dy < 0) ? -dy : dy;
    }

    if (steps == 0)
    {
        touch_draw_point(x1, y1, color);
        return;
    }

    for (i = 0; i <= steps; i++)
    {
        uint16_t px = (uint16_t)((int32_t)x1 + (dx * i) / steps);
        uint16_t py = (uint16_t)((int32_t)y1 + (dy * i) / steps);

        touch_draw_point(px, py, color);
    }
}

int main(void)
{
    uint16_t x;
    uint16_t y;
    uint16_t last_x = 0U;
    uint16_t last_y = 0U;
    uint8_t have_last = 0U;
    uint32_t tick = 0U;

    bsp_init();
    sdram_init();
    lcd_init();
    lcd_display_dir(LTDC_DIR_LANDSCAPE);

    lcd_clear(WHITE);
    lcd_show_string(TOUCH_TEXT_X, 10U, TOUCH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TOUCH_TEXT_X, 30U, TOUCH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "TOUCH TEST", RED);
    lcd_show_string(TOUCH_TEXT_X, 50U, TOUCH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    if (touch_init() != 0U)
    {
        lcd_show_string(TOUCH_TEXT_X, 80U, TOUCH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Touch Init Failed!", RED);
        printf("touch init failed\r\n");
    }
    else
    {
        lcd_show_string(TOUCH_TEXT_X, 80U, TOUCH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Touch Ready!", BLUE);
        printf("touch ready\r\n");
    }

    printf("30_touch ready\r\n");

    for (;;)
    {
        if (touch_scan(0U) != 0U)
        {
            touch_read_xy(&x, &y);

            if ((x < lcd_get_width()) && (y < lcd_get_height()))
            {
                if (have_last != 0U)
                {
                    touch_draw_line(last_x, last_y, x, y, BLUE);
                }

                touch_draw_point(x, y, RED);
                last_x = x;
                last_y = y;
                have_last = 1U;

                printf("touch: x=%u y=%u\r\n", (unsigned)x, (unsigned)y);
            }
        }
        else
        {
            have_last = 0U;
        }

        tick++;

        if ((tick % TOUCH_LED_DIV) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(TOUCH_SAMPLE_MS);
    }
}
