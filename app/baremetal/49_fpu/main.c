/**
 * @file    main.c
 * @brief   49_fpu: Julia fractal rendered to the RGB panel. The per-frame time
 *          is measured with sys_get_tick() and reported on the screen and over
 *          USART1. KEY0/KEY2 step the zoom, WK_UP toggles auto-zoom.
 */

#include <stdio.h>

#include "bsp.h"
#define FPU_ITERATION   128U
#define FPU_REAL_CONST  0.285f
#define FPU_IMG_CONST   0.01f
#define FPU_ROW_MAX     800U

#if defined(__FPU_USED) && (__FPU_USED == 1)
#define FPU_MODE_TEXT "FPU On"
#else
#define FPU_MODE_TEXT "FPU Off"
#endif

static const uint16_t g_zoom_tbl[] =
{
    20U, 40U, 60U, 80U, 100U, 120U, 150U, 200U, 300U, 400U, 600U, 800U, 1200U, 1600U
};
#define FPU_ZOOM_COUNT (sizeof(g_zoom_tbl) / sizeof(g_zoom_tbl[0]))

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

static void julia_row(uint16_t y, uint16_t size_x, uint16_t offset_x,
                      uint16_t offset_y, uint16_t zoom)
{
    uint16_t x;
    uint8_t  i;
    float    tmp1;
    float    tmp2;
    float    num_real;
    float    num_img;
    float    radius;

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
}

static void julia_draw(uint16_t size_x, uint16_t size_y, uint16_t zoom)
{
    uint16_t y;

    for (y = 0U; y < size_y; y++)
    {
        julia_row(y, size_x, (uint16_t)(size_x / 2U), (uint16_t)(size_y / 2U), zoom);
        lcd_color_fill(0U, y, (uint16_t)(size_x - 1U), y, g_row);
    }
}

int main(void)
{
    uint16_t width;
    uint16_t height;
    uint8_t  zoom_idx = 5U;      /* 120 */
    bool     auto_zoom = true;
    uint32_t start;
    uint32_t elapsed;
    char     buf[48];

    bsp_init();
    sdram_init();
    lcd_init();
    lcd_clear(BLACK);
    julia_clut_init();

    width  = lcd_get_width();
    height = lcd_get_height();

    if (width > FPU_ROW_MAX)
    {
        width = FPU_ROW_MAX;
    }

    printf("49_fpu ready (%s) %ux%u\r\n", FPU_MODE_TEXT, (unsigned)width, (unsigned)height);

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY0)
        {
            zoom_idx = (uint8_t)((zoom_idx + 1U) % FPU_ZOOM_COUNT);
            auto_zoom = false;
        }
        else if (key == KEY2)
        {
            zoom_idx = (uint8_t)((zoom_idx + FPU_ZOOM_COUNT - 1U) % FPU_ZOOM_COUNT);
            auto_zoom = false;
        }
        else if (key == KEY_WKUP)
        {
            auto_zoom = !auto_zoom;
        }
        else
        {
            /* no key */
        }

        start = sys_get_tick();
        julia_draw(width, height, g_zoom_tbl[zoom_idx]);
        elapsed = sys_get_tick() - start;

        sprintf(buf, "%s zoom:%u runtime:%lums%s", FPU_MODE_TEXT,
                (unsigned)g_zoom_tbl[zoom_idx], (unsigned long)elapsed,
                auto_zoom ? " AUTO" : "");
        lcd_fill(0U, 0U, (uint16_t)(width - 1U), 15U, BLACK);
        lcd_show_string(2U, 0U, width, 16U, LCD_FONT_SIZE_16, buf, GREEN);
        printf("%s\r\n", buf);

        if (auto_zoom)
        {
            zoom_idx = (uint8_t)((zoom_idx + 1U) % FPU_ZOOM_COUNT);
        }

        led_toggle(LED0);
        delay_ms(50U);
    }
}
