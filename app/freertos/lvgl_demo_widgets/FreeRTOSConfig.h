/**
 * @file    FreeRTOSConfig.h
 * @brief   lvgl_demo_widgets specific FreeRTOS configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stddef.h>

/* Larger heap for the LVGL drawing pool and tasks (common default is 10 KB). */
#define configTOTAL_HEAP_SIZE ((size_t)(64 * 1024))

#include "FreeRTOSConfig_common.h"

#endif /* FREERTOS_CONFIG_H */
