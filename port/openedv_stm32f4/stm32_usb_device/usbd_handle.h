/**
 * @file    usbd_handle.h
 * @brief   USB device stack handle, owned by the port layer (usbd_conf.c).
 *          Applications and the port's class interfaces share this instance.
 */

#ifndef PORT_USBD_HANDLE_H
#define PORT_USBD_HANDLE_H

#include "usbd_core.h"

extern USBD_HandleTypeDef USBD_Device;

#endif /* PORT_USBD_HANDLE_H */
