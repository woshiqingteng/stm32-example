/**
 * @file    usbd_storage_if.c
 * @brief   USB device Mass Storage class interface. Three logical units are
 *          exposed: SPI NOR flash, NAND flash (via the FTL) and the SD card.
 *          The NAND glue is a weak hook overridden by lib_nand_storage, so the
 *          port layer keeps no lib dependency.
 */

#include "usbd_storage_if.h"
#include "sdio.h"
#include "nor.h"

#define STORAGE_LUN_NBR         3U
#define STORAGE_BLK_SIZE        512U

#define LUN_NOR                 0U
#define LUN_NAND                1U
#define LUN_SD                  2U

/* NOR region exposed to the host: the first 25 MB (the FatFs area). */
#define NOR_LUN_SECTOR_COUNT    (25U * 1024U * 2U)

volatile usb_storage_activity_t g_usb_storage_activity = USB_STORAGE_ACTIVITY_IDLE;
volatile usb_storage_error_t    g_usb_storage_error    = USB_STORAGE_ERROR_NONE;

/* Weak NAND hooks: strong versions live in lib_nand_storage. */
__attribute__((weak)) int8_t nand_storage_init(void)                    { return -1; }
__attribute__((weak)) int8_t nand_storage_capacity(uint32_t *block_num, uint16_t *block_size)
{ (void)block_num; (void)block_size; return -1; }
__attribute__((weak)) int8_t nand_storage_read(uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{ (void)buf; (void)blk_addr; (void)blk_len; return -1; }
__attribute__((weak)) int8_t nand_storage_write(uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{ (void)buf; (void)blk_addr; (void)blk_len; return -1; }

/* Mass storage inquiry data, one 36 byte record per logical unit. */
static const int8_t STORAGE_Inquirydata[] = {
    /* LUN 0: NOR */
    0x00, 0x80, 0x02, 0x02,
    (STANDARD_INQUIRY_DATA_LEN - 4), 0x00, 0x00, 0x00,
    'A', 'L', 'I', 'E', 'N', 'T', 'E', 'K',
    'N', 'O', 'R', ' ', 'F', 'l', 'a', 's', 'h', ' ', 'D', 'i', 's', 'k', ' ', ' ',
    '1', '.', '0', ' ',
    /* LUN 1: NAND */
    0x00, 0x80, 0x02, 0x02,
    (STANDARD_INQUIRY_DATA_LEN - 4), 0x00, 0x00, 0x00,
    'A', 'L', 'I', 'E', 'N', 'T', 'E', 'K',
    'N', 'A', 'N', 'D', ' ', 'F', 'l', 'a', 's', 'h', 'D', 'i', 's', 'k', ' ', ' ',
    '1', '.', '0', ' ',
    /* LUN 2: SD */
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
        return nand_storage_init();
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
        return nand_storage_capacity(block_num, block_size);
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
        res = nand_storage_read(buf, blk_addr, blk_len);
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
        res = nand_storage_write(buf, blk_addr, blk_len);
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
