/**
 * @file    usbd_storage_if.h
 * @brief   USB device Mass Storage interface backed by the on-board SD card.
 */

#ifndef PORT_USBD_STORAGE_IF_H
#define PORT_USBD_STORAGE_IF_H

#include <stdint.h>
#include "usbd_msc.h"

/* Bits reported to the application (see usbd_storage_if.c). */
#define USB_STORAGE_WRITING     0x01U
#define USB_STORAGE_READING     0x02U
#define USB_STORAGE_WRITE_ERR   0x04U
#define USB_STORAGE_READ_ERR    0x08U

extern USBD_StorageTypeDef USBD_Storage_Interface_fops;

/** @brief  Last transfer activity, valid between application polls. */
extern volatile uint8_t g_usb_storage_state;

#endif /* PORT_USBD_STORAGE_IF_H */
