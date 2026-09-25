/**
 * @file    usbd_desc.h
 * @brief   USB device descriptors shared by the MSC, CDC (VCP) and AUDIO
 *          device apps.
 */

#ifndef PORT_USBD_DESC_H
#define PORT_USBD_DESC_H

#include "usbd_def.h"

#define DEVICE_ID1          (UID_BASE)
#define DEVICE_ID2          (UID_BASE + 0x4U)
#define DEVICE_ID3          (UID_BASE + 0x8U)

#define USB_SIZ_STRING_SERIAL   0x1AU

extern USBD_DescriptorsTypeDef MSC_Desc;
extern USBD_DescriptorsTypeDef VCP_Desc;
extern USBD_DescriptorsTypeDef AUDIO_Desc;

#endif /* PORT_USBD_DESC_H */
