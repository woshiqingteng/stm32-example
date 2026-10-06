/**
 * @file    FreeRTOSConfig.h
 * @brief   54_usb_device_msc specific FreeRTOS configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* The MSC thread (4 KB stack) and the USB stack allocate from this heap. */
#define configTOTAL_HEAP_SIZE ((size_t)(64 * 1024))

#include "FreeRTOSConfig_common.h"

#endif /* FREERTOS_CONFIG_H */
