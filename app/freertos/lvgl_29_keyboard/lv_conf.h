/**
 * @file    lv_conf.h
 * @brief   lv_29_keyboard specific LVGL configuration.
 *
 * Only settings that differ from lv_conf_common.h are listed here; define them
 * before including the common header.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* LVGL pool in SDRAM: frame buffer occupies 0xC0000000 (4 MB reserved), pool next. */
#define LV_MEM_ADR    0xC0400000U
#define LV_MEM_SIZE   (512U * 1024U)

#include "lv_conf_common.h"

#endif /* LV_CONF_H */
