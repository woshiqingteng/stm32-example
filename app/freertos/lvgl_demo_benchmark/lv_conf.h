/**
 * @file    lv_conf.h
 * @brief   lvgl_demo_benchmark specific LVGL configuration.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_DEMO_BENCHMARK 1

/* The benchmark reports its result through LV_LOG(); route it to printf so the
 * HIL can capture the "Weighted FPS" summary and per-scene CSV lines.  Keep the
 * level at USER so LVGL's own warnings do not flood the UART (LV_LOG() maps to
 * lv_log() and is printed regardless of the level). */
#define LV_USE_LOG     1
#define LV_LOG_PRINTF  1
#define LV_LOG_LEVEL   LV_LOG_LEVEL_USER

/* The benchmark exercises compressed fonts and the 28px Montserrat face. */
#define LV_USE_FONT_COMPRESSED 1
#define LV_FONT_MONTSERRAT_28  1

#include "lv_conf_common.h"

#endif /* LV_CONF_H */
