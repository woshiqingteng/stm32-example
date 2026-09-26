/**
 * @file    usbh_handle.h
 * @brief   USB host stack handle, owned by the port layer (usbh_conf.c).
 *          Applications and the port's MSC diskio glue share this instance.
 */

#ifndef PORT_USBH_HANDLE_H
#define PORT_USBH_HANDLE_H

#include "usbh_core.h"

extern USBH_HandleTypeDef g_hUSBHost;

#endif /* PORT_USBH_HANDLE_H */
