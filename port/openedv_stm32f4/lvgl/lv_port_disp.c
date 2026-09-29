/**
 * @file    lv_port_disp.c
 * @brief   LVGL display port: render into SDRAM and flush via the RGB panel.
 *
 * SDRAM layout: frame buffer at 0xC0000000 (4 MB reserved), LVGL pool at
 * 0xC0400000 (see the app lv_conf.h) and the draw buffer at 0xC0480000.
 * Flushing copies one area with the panel API (DMA2D M2M).
 */

#include <stdio.h>

#include "lvgl.h"
#include "lcd.h"
#include "lv_port_disp.h"

#define LVGL_POOL_ADDR       0xC0400000U
#define LVGL_DRAW_BUF_ADDR   0xC0480000U
#define LVGL_DRAW_BUF_LINES  40U

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
