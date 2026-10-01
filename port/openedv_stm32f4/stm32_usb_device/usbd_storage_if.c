/**
 * @file    usbd_storage_if.c
 * @brief   USB device Mass Storage class interface.  A single logical unit is
 *          exposed: the SD card (a USB card reader).
 *
 * Note: exposing NOR/NAND alongside the SD card made Windows fail to enumerate
 * the composite device (the NAND LUN reports a bogus multi-TB capacity), so only
 * the SD is exposed.
 */

#include "usbd_storage_if.h"
#include "sdio.h"
#include "nor.h"
#include "ftl.h"
#include "nand.h"

#define STORAGE_LUN_NBR         1U
#define STORAGE_BLK_SIZE        512U

#define LUN_NOR                 0xFEU
#define LUN_NAND                0xFFU
#define LUN_SD                  0U

/* NOR region exposed to the host: the first 25 MB (the FatFs area). */
#define NOR_LUN_SECTOR_COUNT    (25U * 1024U * 2U)

volatile usb_storage_activity_t g_usb_storage_activity = USB_STORAGE_ACTIVITY_IDLE;
volatile usb_storage_error_t    g_usb_storage_error    = USB_STORAGE_ERROR_NONE;

/* Mass storage inquiry data, one 36 byte record per logical unit. */
static const int8_t STORAGE_Inquirydata[] = {
    /* LUN 0: SD */
    0x00, 0x80, 0x02, 0x02,
    (STANDARD_INQUIRY_DATA_LEN - 4), 0x00, 0x00, 0x00,
    'A', 'L', 'I', 'E', 'N', 'T', 'E', 'K',
    'S', 'D', ' ', 'C', 'a', 'r', 'd', ' ',
    'D', 'i', 's', 'k', ' ', ' ', ' ', ' ',
    '1', '.', '0', ' ',
};

static int8_t STORAGE_Init(uint8_t lun);
static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady(uint8_t lun);
static int8_t STORAGE_IsWriteProtected(uint8_t lun);
static int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun(void);

USBD_StorageTypeDef USBD_Storage_Interface_fops = {
    STORAGE_Init,
    STORAGE_GetCapacity,
    STORAGE_IsReady,
    STORAGE_IsWriteProtected,
    STORAGE_Read,
    STORAGE_Write,
    STORAGE_GetMaxLun,
    (int8_t *)STORAGE_Inquirydata,
};

static int8_t STORAGE_Init(uint8_t lun)
{
    if (lun == LUN_NOR)
    {
        nor_init();
        return 0;
    }

    if (lun == LUN_NAND)
    {
        return (ftl_init() == 0U) ? 0 : -1;
    }

    return (sdio_init() == 0U) ? 0 : -1;
}

static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    if (lun == LUN_NOR)
    {
        *block_size = STORAGE_BLK_SIZE;
        *block_num  = NOR_LUN_SECTOR_COUNT;
        return 0;
    }

    if (lun == LUN_NAND)
    {
        *block_size = STORAGE_BLK_SIZE;
        *block_num  = (uint32_t)nand_dev.valid_blocknum *
                      nand_dev.block_pagenum *
                      nand_dev.page_mainsize / STORAGE_BLK_SIZE;
        return 0;
    }

    {
        HAL_SD_CardInfoTypeDef info;

        HAL_SD_GetCardInfo(&g_sdcard_handle, &info);
        *block_size = info.LogBlockSize;
        *block_num  = info.LogBlockNbr - 1U;
    }

    return 0;
}

static int8_t STORAGE_IsReady(uint8_t lun)
{
    (void)lun;
    return 0;
}

static int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    (void)lun;
    return 0;
}

static int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    int8_t res;

    g_usb_storage_activity = USB_STORAGE_ACTIVITY_READING;

    if (lun == LUN_NOR)
    {
        uint16_t i;

        for (i = 0U; i < blk_len; i++)
        {
            nor_read(buf + (uint32_t)i * STORAGE_BLK_SIZE,
                     blk_addr * STORAGE_BLK_SIZE + (uint32_t)i * STORAGE_BLK_SIZE,
                     STORAGE_BLK_SIZE);
        }

        return 0;
    }

    if (lun == LUN_NAND)
    {
        res = (ftl_read_sectors(buf, blk_addr, STORAGE_BLK_SIZE, blk_len) == 0U) ? 0 : -1;
    }
    else
    {
        res = (int8_t)sd_read_disk(buf, blk_addr, blk_len);
    }

    if (res != 0)
    {
        g_usb_storage_error = USB_STORAGE_ERROR_READ;
    }

    return res;
}

static int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    int8_t res;

    g_usb_storage_activity = USB_STORAGE_ACTIVITY_WRITING;

    if (lun == LUN_NOR)
    {
        uint16_t i;

        for (i = 0U; i < blk_len; i++)
        {
            nor_write(buf + (uint32_t)i * STORAGE_BLK_SIZE,
                      blk_addr * STORAGE_BLK_SIZE + (uint32_t)i * STORAGE_BLK_SIZE,
                      STORAGE_BLK_SIZE);
        }

        return 0;
    }

    if (lun == LUN_NAND)
    {
        res = (ftl_write_sectors(buf, blk_addr, STORAGE_BLK_SIZE, blk_len) == 0U) ? 0 : -1;
    }
    else
    {
        res = (int8_t)sd_write_disk(buf, blk_addr, blk_len);
    }

    if (res != 0)
    {
        g_usb_storage_error = USB_STORAGE_ERROR_WRITE;
    }

    return res;
}

static int8_t STORAGE_GetMaxLun(void)
{
    return (int8_t)(STORAGE_LUN_NBR - 1U);
}
