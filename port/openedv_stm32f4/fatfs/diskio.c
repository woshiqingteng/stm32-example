/**
 * @file    diskio.c
 * @brief   FatFs physical drive glue for the ALIENTEK F429 board.
 *          Drive 0 is the SD card (SDIO) and drive 1 the on-board SPI NOR flash
 *          (W25Qxx). Drive 2 depends on the port variant: with FATFS_NAND it is
 *          the NAND flash through the FTL (fatfs_nand_port); with FATFS_USB_MSC
 *          it is a USB mass storage device (fatfs_stm32_usb_msc_port).
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
#endif

#ifdef FATFS_NAND
#include "ftl.h"
#include "nand.h"

#define EX_NAND     2   /* NAND flash via the FTL (logical drive "2:") */
#endif

/* NOR flash region handed to FatFs: the first 25 MB of the 32 MB part. */
#define NOR_FATFS_SECTOR_SIZE   512U
#define NOR_FATFS_SECTOR_COUNT  (25U * 1024U * 2U)  /* 25 MB / 512 B */
#define NOR_FATFS_BLOCK_SIZE    8U                  /* 8 sectors = one 4 KB erase block */
#define NOR_FATFS_BASE    0U

#ifdef FATFS_NAND
#define NAND_SECTOR_SIZE        512U
#endif

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
#endif

#ifdef FATFS_NAND
    if (pdrv == EX_NAND)
    {
        return 0;   /* NAND is always ready once the FTL initialises */
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
#endif

#ifdef FATFS_NAND
        case EX_NAND:
            res = (ftl_init() == 0U) ? 0U : 1U;
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
#endif

#ifdef FATFS_NAND
        case EX_NAND:
            return (ftl_read_sectors(buff, (uint32_t)sector, NAND_SECTOR_SIZE, count) == 0U)
                   ? RES_OK : RES_ERROR;
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
#endif

#ifdef FATFS_NAND
        case EX_NAND:
            return (ftl_write_sectors((uint8_t *)buff, (uint32_t)sector, NAND_SECTOR_SIZE, count) == 0U)
                   ? RES_OK : RES_ERROR;
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
#endif
#ifdef FATFS_NAND
    else if (pdrv == EX_NAND)
    {
        switch (cmd)
        {
            case CTRL_SYNC:
                res = RES_OK;
                break;

            case GET_SECTOR_SIZE:
                *(DWORD *)buff = NAND_SECTOR_SIZE;
                res = RES_OK;
                break;

            case GET_BLOCK_SIZE:
                *(DWORD *)buff = (DWORD)(nand_dev.page_mainsize / NAND_SECTOR_SIZE);
                res = RES_OK;
                break;

            case GET_SECTOR_COUNT:
                *(DWORD *)buff = (DWORD)((uint32_t)nand_dev.valid_blocknum *
                                         nand_dev.block_pagenum *
                                         nand_dev.page_mainsize / NAND_SECTOR_SIZE);
                res = RES_OK;
                break;

            default:
                res = RES_PARERR;
                break;
        }
    }
#endif
    else
    {
        res = RES_PARERR;
    }

    return res;
}
