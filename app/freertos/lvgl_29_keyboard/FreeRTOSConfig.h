/**
 * @file    FreeRTOSConfig.h
 * @brief   lvgl_29_keyboard specific FreeRTOS configuration.
 *
 * Only settings that differ from FreeRTOSConfig_common.h are listed here;
 * define them before including the common header.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stddef.h>

/* Larger heap for the LVGL and LED tasks (common default is 10 KB). */
#define configTOTAL_HEAP_SIZE ((size_t)(48 * 1024))

#include "FreeRTOSConfig_common.h"

#endif /* FREERTOS_CONFIG_H */
