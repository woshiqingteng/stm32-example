/**
 * @file    usbh_diskio.c
 * @brief   FatFs physical drive glue for a USB mass storage device. The FatFs
 *          layer is told to route one logical drive here (see the USB MSC
 *          mapping in port/fatfs_port/diskio.c).
 */

#include "usbh_diskio.h"
#include "usbh_core.h"
#include "usbh_msc.h"

#define USB_DEFAULT_BLOCK_SIZE   512U

/* USB host handle owned by the application. */
extern USBH_HandleTypeDef g_hUSBHost;

DSTATUS USBH_initialize(void)
{
    return 0;
}

DSTATUS USBH_status(void)
{
    MSC_HandleTypeDef *msc = (MSC_HandleTypeDef *)g_hUSBHost.pActiveClass->pData;

    if (USBH_MSC_UnitIsReady(&g_hUSBHost, msc->current_lun) != 0U)
    {
        return 0;
    }

    return STA_NOINIT;
}

DRESULT USBH_read(BYTE *buff, DWORD sector, UINT count)
{
    MSC_HandleTypeDef *msc = (MSC_HandleTypeDef *)g_hUSBHost.pActiveClass->pData;

    if (USBH_MSC_Read(&g_hUSBHost, msc->current_lun, sector, buff, count) == USBH_OK)
    {
        return RES_OK;
    }

    return RES_ERROR;
}

DRESULT USBH_write(const BYTE *buff, DWORD sector, UINT count)
{
    MSC_HandleTypeDef *msc = (MSC_HandleTypeDef *)g_hUSBHost.pActiveClass->pData;

    if (USBH_MSC_Write(&g_hUSBHost, msc->current_lun, sector, (BYTE *)buff, count) == USBH_OK)
    {
        return RES_OK;
    }

    return RES_ERROR;
}

DRESULT USBH_ioctl(BYTE cmd, void *buff)
{
    MSC_LUNTypeDef     info;
    MSC_HandleTypeDef *msc = (MSC_HandleTypeDef *)g_hUSBHost.pActiveClass->pData;

    switch (cmd)
    {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT:
            if (USBH_MSC_GetLUNInfo(&g_hUSBHost, msc->current_lun, &info) == USBH_OK)
            {
                *(DWORD *)buff = info.capacity.block_nbr;
                return RES_OK;
            }
            break;

        case GET_SECTOR_SIZE:
            if (USBH_MSC_GetLUNInfo(&g_hUSBHost, msc->current_lun, &info) == USBH_OK)
            {
                *(DWORD *)buff = info.capacity.block_size;
                return RES_OK;
            }
            break;

        case GET_BLOCK_SIZE:
            if (USBH_MSC_GetLUNInfo(&g_hUSBHost, msc->current_lun, &info) == USBH_OK)
            {
                *(DWORD *)buff = info.capacity.block_size / USB_DEFAULT_BLOCK_SIZE;
                return RES_OK;
            }
            break;

        default:
            return RES_PARERR;
    }

    return RES_ERROR;
}
