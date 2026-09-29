/**
 * @file    lvgl_tick.c
 * @brief   LVGL tick hook storage (defined inside the lvgl module).
 */

#include "lvgl_tick.h"

static uint32_t lvgl_tick_default(void)
{
    return 0U;
}

lvgl_tick_fn_t g_lvgl_tick_fn = lvgl_tick_default;

void lvgl_tick_set(lvgl_tick_fn_t fn)
{
    if (fn != 0)
    {
        g_lvgl_tick_fn = fn;
    }
}
