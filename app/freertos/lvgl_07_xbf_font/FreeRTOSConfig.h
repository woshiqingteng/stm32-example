/**
 * @file    FreeRTOSConfig.h
 * @brief   lvgl_07_xbf_font specific FreeRTOS configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stddef.h>

#define configTOTAL_HEAP_SIZE ((size_t)(16 * 1024))

#include "FreeRTOSConfig_common.h"

#endif /* FREERTOS_CONFIG_H */
