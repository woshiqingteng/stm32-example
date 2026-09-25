/**
 * @file    usbh_conf.h
 * @brief   USB Host low level configuration for the ST USB host stack (3.5.3).
 *          The host core headers include this file, so the port layer hands its
 *          directory to the usb_host target at configure time.
 */

#ifndef PORT_USBH_CONF_H
#define PORT_USBH_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hcd.h"

#ifndef HAL_HCD_MODULE_ENABLED
#define HAL_HCD_MODULE_ENABLED
#endif

#define USBH_MAX_NUM_ENDPOINTS          2U
#define USBH_MAX_NUM_INTERFACES         2U
#define USBH_MAX_NUM_CONFIGURATION      1U
#define USBH_MAX_NUM_SUPPORTED_CLASS    1U
#define USBH_KEEP_CFG_DESCRIPTOR        0U
#define USBH_MAX_SIZE_CONFIGURATION     0x200U
#define USBH_MAX_DATA_BUFFER            0x200U
#define USBH_DEBUG_LEVEL                0U
#define USBH_USE_OS                     0U

/* OTG instance selector used by USBH_Init(). */
#define HOST_HS                         0U
#define HOST_FS                         1U

/*
 * The host stack allocates one class handle. A single static arena is enough
 * for a host app and avoids pulling in a heap implementation.
 */
void  *usbh_static_malloc(uint32_t size);
void   usbh_static_free(void *p);

#define USBH_malloc         usbh_static_malloc
#define USBH_free           usbh_static_free
#define USBH_memset         memset
#define USBH_memcpy         memcpy

#if (USBH_DEBUG_LEVEL > 0U)
#define USBH_UsrLog(...)    do { printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_UsrLog(...)    do { } while (0)
#endif

#if (USBH_DEBUG_LEVEL > 1U)
#define USBH_ErrLog(...)    do { printf("ERROR: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_ErrLog(...)    do { } while (0)
#endif

#if (USBH_DEBUG_LEVEL > 2U)
#define USBH_DbgLog(...)    do { printf("DEBUG: "); printf(__VA_ARGS__); printf("\n"); } while (0)
#else
#define USBH_DbgLog(...)    do { } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* PORT_USBH_CONF_H */
