/**
 * @file    lv_conf.h
 * @brief   lvgl_36_spinner specific LVGL configuration.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* LVGL pool in SDRAM: frame buffer at 0xC0000000 (4 MB reserved). */
#define LV_MEM_ADR    0xC0400000U
#define LV_MEM_SIZE   (512U * 1024U)

#include "lv_conf_common.h"

#endif /* LV_CONF_H */
