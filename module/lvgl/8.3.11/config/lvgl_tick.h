/**
 * @file    lvgl_tick.h
 * @brief   LVGL millisecond tick hook.
 *
 * The function pointer is defined inside the lvgl module (see lvgl_tick.c) and
 * is injected by the board port (port/openedv_stm32f4/lvgl). This keeps the
 * module independent of any board or OS layer.
 */

#ifndef LVGL_TICK_H
#define LVGL_TICK_H

#include <stdint.h>

typedef uint32_t (*lvgl_tick_fn_t)(void);

extern lvgl_tick_fn_t g_lvgl_tick_fn;

void lvgl_tick_set(lvgl_tick_fn_t fn);

#endif /* LVGL_TICK_H */
