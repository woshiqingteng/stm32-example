/**
 * @file    lv_conf.h
 * @brief   lvgl_51_filemgr specific LVGL configuration.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_MEM_ADR    0xC0400000U
#define LV_MEM_SIZE   (512U * 1024U)

#define LV_USE_BMP 1
#define LV_USE_SJPG 1
#define LV_USE_QRCODE 1

#include "lv_conf_common.h"

#endif /* LV_CONF_H */
