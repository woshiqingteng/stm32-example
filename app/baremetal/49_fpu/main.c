/**
 * @file    main.c
 * @brief   49_fpu: Julia fractal rendered to the RGB panel. The per-frame time
 *          is measured with a 10 kHz basic timer (0.1 ms) and reported on the
 *          screen and over USART1. KEY0/KEY2 step the zoom, WK_UP toggles
 *          auto-zoom.
 */

#include <stdio.h>

#include "bsp.h"
#include "btim.h"
#include "lcd.h"
#include "sdram.h"

#define FPU_ITERATION_COUNT   128U
#define FPU_REAL_CONST  0.285f
#define FPU_IMG_CONST   0.01f
#define FPU_ROW_COUNT     800U
#define FPU_TIMER_PSC   (9000U - 1U) /* 90 MHz / 9000 = 10 kHz (0.1 ms) */

#if defined(__FPU_USED) && (__FPU_USED == 1)
#define FPU_MODE_TEXT "FPU On"
#else
#define FPU_MODE_TEXT "FPU Off"
#endif

static const uint16_t g_zoom_tbl[] =
{
    120U, 110U, 100U, 150U, 200U, 275U, 350U, 450U,
    600U, 800U, 1000U, 1200U, 1500U, 2000U, 1500U,
    1200U, 1000U, 800U, 600U, 450U, 350U, 275U, 200U,
    150U, 100U, 110U,
};
#define FPU_ZOOM_COUNT (sizeof(g_zoom_tbl) / sizeof(g_zoom_tbl[0]))

static uint16_t g_color_map[FPU_ITERATION_COUNT];
static uint16_t g_row[FPU_ROW_COUNT];
static volatile uint8_t g_timeout;

static void on_tim6(void)
{
    g_timeout++;
}

static void julia_clut_init(void)
{
    uint32_t i;
    uint16_t red;
    uint16_t green;
    uint16_t blue;

    for (i = 0U; i < FPU_ITERATION_COUNT; i++)
    {
        red   = (uint16_t)(((i * 8U * 256U / FPU_ITERATION_COUNT) % 256U) >> 3U);
        green = (uint16_t)(((i * 6U * 256U / FPU_ITERATION_COUNT) % 256U) >> 2U);
        blue  = (uint16_t)(((i * 4U * 256U / FPU_ITERATION_COUNT) % 256U) >> 3U);

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

        while ((i < (FPU_ITERATION_COUNT - 1U)) && (radius < 4.0f))
        {
            tmp1     = num_real * num_real;
            tmp2     = num_img * num_img;
            num_img  = (2.0f * num_real * num_img) + FPU_IMG_CONST;
            num_real = tmp1 - tmp2 + FPU_REAL_CONST;
            radius   = tmp1 + tmp2;
            i++;
        }

        g_row[size_x - x - 1U] = g_color_map[i];
    }
}

static void julia_draw(uint16_t size_x, uint16_t size_y, uint16_t zoom)
{
    uint16_t y;

    for (y = 0U; y < size_y; y++)
    {
        julia_row(y, size_x, (uint16_t)(size_x / 2U), (uint16_t)(size_y / 2U), zoom);
        lcd_blit(0U, y, (uint16_t)(size_x - 1U), y, g_row);
    }
}

int main(void)
{
    uint16_t width;
    uint16_t height;
    uint8_t  zoom_idx = 0U;      /* 120 */
    bool     auto_zoom = false;
    uint32_t ticks;
    char     buf[48];

    bsp_init();
    printf(APP_BANNER "\r\n");
    sdram_init();
    lcd_init();
    lcd_clear(BLACK);
    julia_clut_init();

    /* ARR=65535 -> 65536*0.1 ms = 6.55 s overflow; ticks/10 = ms */
    btim_timx_int_init(65535U, (uint16_t)FPU_TIMER_PSC);
    btim_timx_int_register(&on_tim6);

    width  = lcd_info()->width;
    height = lcd_info()->height;

    if (width > FPU_ROW_COUNT)
    {
        width = FPU_ROW_COUNT;
    }

    lcd_show_string(30U, 50U, 200U, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(30U, 70U, 200U, 16U, LCD_FONT_SIZE_16, "FPU TEST", RED);
    lcd_show_string(30U, 90U, 200U, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30U, 110U, 200U, 16U, LCD_FONT_SIZE_16, "KEY0:+    KEY1:-", RED);
    lcd_show_string(30U, 130U, 200U, 16U, LCD_FONT_SIZE_16, "KEY_UP:AUTO/MANUL", RED);
    delay_ms(500U);

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

        if (auto_zoom)
        {
            zoom_idx = (uint8_t)((zoom_idx + 1U) % FPU_ZOOM_COUNT);
            led_on(LED1);
        }
        else
        {
            led_off(LED1);
        }

        TIM6->CNT = 0U;
        g_timeout = 0U;
        julia_draw(width, height, g_zoom_tbl[zoom_idx]);
        ticks = (uint32_t)TIM6->CNT + ((uint32_t)g_timeout * 65536U);

        sprintf(buf, "%s zoom:%u runtime:%lu.%lums", FPU_MODE_TEXT,
                (unsigned)g_zoom_tbl[zoom_idx],
                (unsigned long)(ticks / 10U), (unsigned long)(ticks % 10U));
        lcd_fill(0U, (uint16_t)(height - 17U), (uint16_t)(width - 1U), (uint16_t)(height - 5U), BLACK);
        lcd_show_string(5U, (uint16_t)(height - 17U), (uint16_t)(width - 5U), 12U,
                        LCD_FONT_SIZE_12, buf, RED);
        printf("%s\r\n", buf);

        led_toggle(LED0);
        delay_ms(50U);
    }
}
