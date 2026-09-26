/**
 * @file    diskio.c
 * @brief   FatFs physical drive glue for the ALIENTEK F429 board.
 *          Drive 0 maps to the SD card (SDIO), drive 1 maps to the on-board SPI
 *          NOR flash (W25Qxx) and, when FATFS_USB_MSC is defined by the
 *          fatfs_stm32_usb_msc_port variant, drive 2 maps to a USB mass storage device
 *          enumerated by
 *          the USB host stack (port/openedv_stm32f4/stm32_usb_host/usbh_diskio.c). The NAND drive is not
 *          wired here because the raw NAND needs the vendor FTL for the
 *          erase-before-write mapping.
 */

#include "ff.h"
#include "diskio.h"
#include "sdio.h"
#include "nor.h"

#define SD_CARD     0   /* SD card (logical drive "0:") */
#define EX_FLASH    1   /* SPI NOR flash (logical drive "1:") */

#ifdef FATFS_USB_MSC
#include "usbh_diskio.h"
#define USB_MSC     2   /* USB mass storage (logical drive "2:") */
#else
#define EX_NAND     2   /* NAND flash via the FTL (logical drive "2:") */
#endif

/*
 * Weak NAND hooks: the base port must not depend on the FTL (which lives in
 * the lib layer). Apps that expose the NAND drive link lib_nand_storage, whose
 * strong definitions override these stubs.
 */
#ifndef FATFS_USB_MSC
__attribute__((weak)) DSTATUS nand_disk_status(void)                 { return STA_NOINIT; }
__attribute__((weak)) DSTATUS nand_disk_initialize(void)             { return STA_NOINIT; }
__attribute__((weak)) DRESULT nand_disk_read(BYTE *buff, LBA_t sector, UINT count)
{ (void)buff; (void)sector; (void)count; return RES_ERROR; }
__attribute__((weak)) DRESULT nand_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{ (void)buff; (void)sector; (void)count; return RES_ERROR; }
__attribute__((weak)) DRESULT nand_disk_ioctl(BYTE cmd, void *buff)
{ (void)cmd; (void)buff; return RES_PARERR; }
#endif

/* NOR flash region handed to FatFs: the first 25 MB of the 32 MB part. */
#define NOR_FATFS_SECTOR_SIZE   512U
#define NOR_FATFS_SECTOR_COUNT  (25U * 1024U * 2U)  /* 25 MB / 512 B */
#define NOR_FATFS_BLOCK_SIZE    8U                  /* 8 sectors = one 4 KB erase block */
#define NOR_FATFS_BASE    0U

DSTATUS disk_status(BYTE pdrv)
{
    if ((pdrv == SD_CARD) || (pdrv == EX_FLASH))
    {
        return 0;
    }

#ifdef FATFS_USB_MSC
    if (pdrv == USB_MSC)
    {
        return USBH_status();
    }
#else
    if (pdrv == EX_NAND)
    {
        return nand_disk_status();
    }
#endif

    return STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    uint8_t res = 0U;

    switch (pdrv)
    {
        case SD_CARD:
            res = sdio_init();
            break;

        case EX_FLASH:
            nor_init();
            break;

#ifdef FATFS_USB_MSC
        case USB_MSC:
            res = (uint8_t)USBH_initialize();
            break;
#else
        case EX_NAND:
            res = (nand_disk_initialize() == 0U) ? 0U : 1U;
            break;
#endif

        default:
            res = 1U;
            break;
    }

    return (res != 0U) ? STA_NOINIT : 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t res = 0U;

    if (count == 0U)
    {
        return RES_PARERR;
    }

    switch (pdrv)
    {
        case SD_CARD:
            res = sd_read_disk(buff, (uint32_t)sector, count);
            break;

        case EX_FLASH:
            while (count-- != 0U)
            {
                nor_read(buff, NOR_FATFS_BASE + (uint32_t)sector * NOR_FATFS_SECTOR_SIZE,
                              (uint16_t)NOR_FATFS_SECTOR_SIZE);
                sector++;
                buff += NOR_FATFS_SECTOR_SIZE;
            }
            break;

#ifdef FATFS_USB_MSC
        case USB_MSC:
            return USBH_read(buff, (DWORD)sector, count);
#else
        case EX_NAND:
            return nand_disk_read(buff, sector, count);
#endif

        default:
            return RES_PARERR;
    }

    return (res == 0U) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t res = 0U;

    if (count == 0U)
    {
        return RES_PARERR;
    }

    switch (pdrv)
    {
        case SD_CARD:
            res = sd_write_disk((uint8_t *)buff, (uint32_t)sector, count);
            break;

        case EX_FLASH:
            while (count-- != 0U)
            {
                nor_write((uint8_t *)buff,
                               NOR_FATFS_BASE + (uint32_t)sector * NOR_FATFS_SECTOR_SIZE,
                               (uint16_t)NOR_FATFS_SECTOR_SIZE);
                sector++;
                buff += NOR_FATFS_SECTOR_SIZE;
            }
            break;

#ifdef FATFS_USB_MSC
        case USB_MSC:
            return USBH_write(buff, (DWORD)sector, count);
#else
        case EX_NAND:
            return nand_disk_write(buff, sector, count);
#endif

        default:
            return RES_PARERR;
    }

    return (res == 0U) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DRESULT res = RES_ERROR;

    if (pdrv == SD_CARD)
    {
        switch (cmd)
        {
            case CTRL_SYNC:
                res = RES_OK;
                break;

            case GET_SECTOR_SIZE:
                *(DWORD *)buff = 512U;
                res = RES_OK;
                break;

            case GET_BLOCK_SIZE:
                *(DWORD *)buff = 1U;
                res = RES_OK;
                break;

            case GET_SECTOR_COUNT:
                *(DWORD *)buff = (DWORD)g_sd_card_info_handle.LogBlockNbr;
                res = RES_OK;
                break;

            default:
                res = RES_PARERR;
                break;
        }
    }
    else if (pdrv == EX_FLASH)
    {
        switch (cmd)
        {
            case CTRL_SYNC:
                res = RES_OK;
                break;

            case GET_SECTOR_SIZE:
                *(DWORD *)buff = NOR_FATFS_SECTOR_SIZE;
                res = RES_OK;
                break;

            case GET_BLOCK_SIZE:
                *(DWORD *)buff = NOR_FATFS_BLOCK_SIZE;
                res = RES_OK;
                break;

            case GET_SECTOR_COUNT:
                *(DWORD *)buff = NOR_FATFS_SECTOR_COUNT;
                res = RES_OK;
                break;

            default:
                res = RES_PARERR;
                break;
        }
    }
#ifdef FATFS_USB_MSC
    else if (pdrv == USB_MSC)
    {
        res = USBH_ioctl(cmd, buff);
    }
#else
    else if (pdrv == EX_NAND)
    {
        res = nand_disk_ioctl(cmd, buff);
    }
#endif
    else
    {
        res = RES_PARERR;
    }

    return res;
}
