/**
 * @file    FreeRTOSConfig.h
 * @brief   lvgl_42_png specific FreeRTOS configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stddef.h>

#define configTOTAL_HEAP_SIZE ((size_t)(64 * 1024))

#include "FreeRTOSConfig_common.h"

#endif /* FREERTOS_CONFIG_H */
