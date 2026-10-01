/**
 * @file    lv_conf.h
 * @brief   lvgl_demo_benchmark specific LVGL configuration.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* LVGL pool in SDRAM: frame buffer at 0xC0000000 (4 MB reserved). */
#define LV_MEM_ADR    0xC0400000U
#define LV_MEM_SIZE   (512U * 1024U)

#define LV_USE_DEMO_BENCHMARK 1

/* The benchmark reports its result through LV_LOG(); route it to printf so the
 * HIL can capture the "Weighted FPS" summary and per-scene CSV lines.  Keep the
 * level at USER so LVGL's own warnings do not flood the UART (LV_LOG() maps to
 * lv_log() and is printed regardless of the level). */
#define LV_USE_LOG     1
#define LV_LOG_PRINTF  1
#define LV_LOG_LEVEL   LV_LOG_LEVEL_USER

#include "lv_conf_common.h"

#endif /* LV_CONF_H */
