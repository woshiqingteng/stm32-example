/**
 * @file    FreeRTOSConfig.h
 * @brief   56_usb_device_cdc specific FreeRTOS configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* The USB stack allocates its tasks/queues from the FreeRTOS heap. */
#define configTOTAL_HEAP_SIZE ((size_t)(32 * 1024))

#include "FreeRTOSConfig_common.h"

#endif /* FREERTOS_CONFIG_H */
