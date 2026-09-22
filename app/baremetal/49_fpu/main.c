/**
 * @file    main.c
 * @brief   49_fpu: Julia fractal benchmark, rendered on the RGB panel. The
 *          per-frame time is measured with sys_get_tick() and reported over
 *          USART1 and the LCD.
 */

#include <stdio.h>
#include "bsp.h"

#define FPU_ITERATION   128U
#define FPU_REAL_CONST  0.285f
#define FPU_IMG_CONST   0.01f
#define FPU_ROW_MAX     800U
#define FPU_ZOOM        120U
#define FPU_TEXT_SIZE   LCD_FONT_SIZE_12
#define FPU_STATUS_H    20U

#if defined(__FPU_USED) && (__FPU_USED == 1)
#define FPU_MODE_TEXT "FPU On"
#else
#define FPU_MODE_TEXT "FPU Off"
#endif

static uint16_t g_color_map[FPU_ITERATION];
static uint16_t g_row[FPU_ROW_MAX];

static void julia_clut_init(void)
{
    uint32_t i;
    uint16_t red;
    uint16_t green;
    uint16_t blue;

    for (i = 0U; i < FPU_ITERATION; i++)
    {
        red   = (uint16_t)(((i * 8U * 256U / FPU_ITERATION) % 256U) >> 3U);
        green = (uint16_t)(((i * 6U * 256U / FPU_ITERATION) % 256U) >> 2U);
        blue  = (uint16_t)(((i * 4U * 256U / FPU_ITERATION) % 256U) >> 3U);

        g_color_map[i] = (uint16_t)((red << 11U) | (green << 5U) | blue);
    }
}

static void julia_generate(uint16_t size_x, uint16_t size_y, uint16_t offset_x,
                           uint16_t offset_y, uint16_t zoom)
{
    uint16_t x;
    uint16_t y;
    uint8_t  i;
    float    tmp1;
    float    tmp2;
    float    num_real;
    float    num_img;
    float    radius;

    for (y = 0U; y < size_y; y++)
    {
        for (x = 0U; x < size_x; x++)
        {
            num_real = ((float)y - (float)offset_y) / (float)zoom;
            num_img  = ((float)x - (float)offset_x) / (float)zoom;
            i        = 0U;
            radius   = 0.0f;

            while ((i < (FPU_ITERATION - 1U)) && (radius < 4.0f))
            {
                tmp1     = num_real * num_real;
                tmp2     = num_img * num_img;
                num_img  = (2.0f * num_real * num_img) + FPU_IMG_CONST;
                num_real = tmp1 - tmp2 + FPU_REAL_CONST;
                radius   = tmp1 + tmp2;
                i++;
            }

            g_row[x] = g_color_map[i];
        }

        lcd_color_fill(0U, y, (uint16_t)(size_x - 1U), y, g_row);
    }
}

int main(void)
{
    uint32_t start;
    uint32_t elapsed;
    char     buf[48];

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(BLACK);
    julia_clut_init();

    printf("49_fpu ready (%s)\r\n", FPU_MODE_TEXT);

    for (;;)
    {
        start = sys_get_tick();
        julia_generate(lcd_get_width(), lcd_get_height(),
                       (uint16_t)(lcd_get_width() / 2U), (uint16_t)(lcd_get_height() / 2U),
                       FPU_ZOOM);
        elapsed = sys_get_tick() - start;

        sprintf(buf, "%s Julia zoom:%u runtime:%lums", FPU_MODE_TEXT, (unsigned)FPU_ZOOM,
                (unsigned long)elapsed);
        lcd_show_string(5U, (uint16_t)(lcd_get_height() - FPU_STATUS_H), 400U,
                        FPU_TEXT_SIZE, FPU_TEXT_SIZE, buf, RED);
        printf("%s\r\n", buf);

        led_toggle(LED0);
        delay_ms(50U);
    }
}
