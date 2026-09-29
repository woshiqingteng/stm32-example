/**
 * @file    lv_port_indev.c
 * @brief   LVGL input port: GT9147 capacitive touch panel.
 */

#include "lvgl.h"
#include "touch.h"
#include "lv_port_indev.h"

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv;

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
