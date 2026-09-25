/**
 * @file    usbh_diskio.h
 * @brief   FatFs physical drive glue for a USB mass storage device enumerated
 *          by the USB host stack.
 */

#ifndef PORT_USBH_DISKIO_H
#define PORT_USBH_DISKIO_H

#include "ff.h"
#include "diskio.h"

DSTATUS USBH_initialize(void);
DSTATUS USBH_status(void);
DRESULT USBH_read(BYTE *buff, DWORD sector, UINT count);
DRESULT USBH_write(const BYTE *buff, DWORD sector, UINT count);
DRESULT USBH_ioctl(BYTE cmd, void *buff);

#endif /* PORT_USBH_DISKIO_H */
