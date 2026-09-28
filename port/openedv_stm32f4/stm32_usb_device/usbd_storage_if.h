/**
 * @file    usbd_storage_if.h
 * @brief   USB device Mass Storage interface backed by the on-board SD card.
 */

#ifndef PORT_USBD_STORAGE_IF_H
#define PORT_USBD_STORAGE_IF_H

#include <stdint.h>

#include "usbd_msc.h"

/** @brief  Current storage activity, reported to the application. */
typedef enum
{
    USB_STORAGE_ACTIVITY_IDLE = 0,
    USB_STORAGE_ACTIVITY_READING,
    USB_STORAGE_ACTIVITY_WRITING
} usb_storage_activity_t;

/** @brief  Last transfer error, cleared after each report. */
typedef enum
{
    USB_STORAGE_ERROR_NONE = 0,
    USB_STORAGE_ERROR_READ,
    USB_STORAGE_ERROR_WRITE
} usb_storage_error_t;

extern USBD_StorageTypeDef USBD_Storage_Interface_fops;

/** @brief  Last transfer activity, valid between application polls. */
extern volatile usb_storage_activity_t g_usb_storage_activity;

/** @brief  Pending transfer error (read-and-clear by the application). */
extern volatile usb_storage_error_t g_usb_storage_error;

#endif /* PORT_USBD_STORAGE_IF_H */
