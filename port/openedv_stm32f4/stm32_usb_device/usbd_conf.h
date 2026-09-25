/**
 * @file    usbd_conf.h
 * @brief   USB Device low level configuration for the ST USB device stack
 *          (2.11.4). The device core headers include this file, so the port
 *          layer hands its directory to the stm32_usb_device target at configure time.
 */

#ifndef PORT_USBD_CONF_H
#define PORT_USBD_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_pcd.h"
#include "stm32f4xx_hal_pcd_ex.h"

#ifndef HAL_PCD_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED
#endif

/* Common USB device configuration. */
#define USBD_MAX_NUM_INTERFACES         1U
#define USBD_MAX_NUM_CONFIGURATION      1U
#define USBD_MAX_STR_DESC_SIZ           0x100U
#define USBD_SELF_POWERED               1U
#define USBD_DEBUG_LEVEL                0U
#define USBD_SUPPORT_USER_STRING_DESC   0U

/* Class configuration. */
#define MSC_MEDIA_PACKET                512U
#define USBD_CDC_INTERVAL               2000U

/* OTG instance selector used by USBD_Init(). */
#define DEVICE_FS                       0U
#define DEVICE_HS                       1U

/* USB connection state, maintained by usbd_conf.c: false = disconnected. */
extern volatile bool g_device_state;

/*
 * The device stack allocates one class handle. Only a single class is
 * registered per device app, so a single static arena is enough and no heap is
 * needed. usbd_conf.c sizes it for the largest class handle.
 */
void  *usbd_static_malloc(uint32_t size);
void   usbd_static_free(void *p);

#define USBD_malloc         usbd_static_malloc
#define USBD_free           usbd_static_free
#define USBD_memset         memset
#define USBD_memcpy         memcpy
#define USBD_Delay          HAL_Delay

#if (USBD_DEBUG_LEVEL > 0U)
#define USBD_UsrLog(...)    do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBD_UsrLog(...)    do { } while (0)
#endif

#if (USBD_DEBUG_LEVEL > 1U)
#define USBD_ErrLog(...)    do { printf("ERROR: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBD_ErrLog(...)    do { } while (0)
#endif

#if (USBD_DEBUG_LEVEL > 2U)
#define USBD_DbgLog(...)    do { printf("DEBUG: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBD_DbgLog(...)    do { } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* PORT_USBD_CONF_H */
