/**
 * @file    lv_port.c
 * @brief   LVGL port: RGB/LTDC display, GT9147 touch input and OS tick.
 *
 * Display: the frame buffer lives at 0xC0000000, the LVGL pool at 0xC0400000
 * (see the app lv_conf.h) and the draw buffer at 0xC0480000; flushing copies
 * one area with the panel API (DMA2D M2M).
 *
 * Tick: lv_conf_common.h sets LV_TICK_CUSTOM to call lv_port_tick_get(). The
 * USE_FREERTOS switch stays here, so the lvgl module keeps no OS dependency.
 */

#include <stdio.h>

#include "lvgl.h"
#include "lcd.h"
#include "touch.h"
#include "delay.h"

#include "lv_port.h"

#if (LV_MEM_ADR == 0)
#error "lv_port requires an explicit SDRAM LV_MEM_ADR (see lv_conf_common.h)"
#endif

/* Derived from the LVGL pool so there is a single source of truth: the pool
 * starts at LV_MEM_ADR, the draw buffer right after it. */
#define LVGL_POOL_ADDR       ((uint32_t)LV_MEM_ADR)
#define LVGL_DRAW_BUF_ADDR   ((uint32_t)(LV_MEM_ADR + LV_MEM_SIZE))
#define LVGL_DRAW_BUF_LINES  40U

#if USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#else
#include "stm32f4xx_hal.h"
#endif

/* ==================== display ==================== */

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    lcd_blit((uint16_t)area->x1, (uint16_t)area->y1,
             (uint16_t)area->x2, (uint16_t)area->y2,
             (const uint16_t *)color_p);

    lv_disp_flush_ready(disp_drv);
}

void lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t draw_buf;
    static lv_disp_drv_t disp_drv;

    lcd_init();
    lcd_display_dir(LCD_DIR_LANDSCAPE);

    /* Guard: the panel frame buffer must stay below the LVGL pool. */
    if ((lcd_info()->framebuf +
         ((uint32_t)lcd_info()->width * lcd_info()->height * lcd_info()->pixsize)) > LVGL_POOL_ADDR)
    {
        printf("lvgl: framebuffer overlaps pool\r\n");
    }

    lv_disp_draw_buf_init(&draw_buf, (void *)LVGL_DRAW_BUF_ADDR, NULL,
                          (uint32_t)lcd_info()->width * LVGL_DRAW_BUF_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = lcd_info()->width;
    disp_drv.ver_res  = lcd_info()->height;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

/* ==================== input ==================== */

/* Test hook (used by the HIL): when g_lv_indev_test_en is non-zero the indev
 * reports the injected point/state instead of the GT9147.  Disabled by default
 * and resolved by symbol (see test/hil/lvgl_verify.py). */
volatile uint8_t g_lv_indev_test_en;
volatile uint8_t g_lv_indev_test_pr;
volatile int32_t g_lv_indev_test_x;
volatile int32_t g_lv_indev_test_y;

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv;

    if (g_lv_indev_test_en)
    {
        data->point.x = (lv_coord_t)g_lv_indev_test_x;
        data->point.y = (lv_coord_t)g_lv_indev_test_y;
        data->state   = g_lv_indev_test_pr ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        return;
    }

    if (touch_scan(false))
    {
        uint16_t x;
        uint16_t y;

        touch_read_xy(&x, &y);
        data->point.x = (lv_coord_t)x;
        data->point.y = (lv_coord_t)y;
        data->state   = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    touch_init();

    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

/* ==================== tick ==================== */

uint32_t lv_port_tick_get(void)
{
#if USE_FREERTOS
    return (uint32_t)xTaskGetTickCount();
#else
    return (uint32_t)HAL_GetTick();
#endif
}
