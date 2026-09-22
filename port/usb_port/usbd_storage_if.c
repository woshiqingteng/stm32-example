/**
 * @file    usbd_storage_if.c
 * @brief   USB device Mass Storage class interface. A single logical unit is
 *          exposed, mapped onto the SD card through the board SDIO driver.
 */

#include "usbd_storage_if.h"
#include "sdio.h"

#define STORAGE_LUN_NBR         1U
#define STORAGE_BLK_SIZE        512U

volatile uint8_t g_usb_storage_state = 0U;

/* Mass storage inquiry data, one 36 byte record per logical unit. */
static const int8_t STORAGE_Inquirydata[] = {
    /* LUN 0 */
    0x00, 0x80, 0x02, 0x02,
    (STANDARD_INQUIRY_DATA_LEN - 4), 0x00, 0x00, 0x00,
    /* Vendor identification (8 bytes) */
    'A', 'L', 'I', 'E', 'N', 'T', 'E', 'K',
    /* Product identification (16 bytes) */
    'S', 'D', ' ', 'C', 'a', 'r', 'd', ' ',
    'D', 'i', 's', 'k', ' ', ' ', ' ', ' ',
    /* Product revision level (4 bytes) */
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
    (void)lun;
    return (sdio_init() == 0U) ? 0 : -1;
}

static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    HAL_SD_CardInfoTypeDef info;

    (void)lun;

    HAL_SD_GetCardInfo(&g_sdcard_handle, &info);
    *block_size = info.LogBlockSize;
    *block_num  = info.LogBlockNbr - 1U;

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
    uint8_t res;

    (void)lun;
    g_usb_storage_state = USB_STORAGE_READING;

    res = sd_read_disk(buf, blk_addr, blk_len);

    if (res != 0U)
    {
        g_usb_storage_state |= USB_STORAGE_READ_ERR;
    }

    return (res != 0U) ? -1 : 0;
}

static int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    uint8_t res;

    (void)lun;
    g_usb_storage_state = USB_STORAGE_WRITING;

    res = sd_write_disk(buf, blk_addr, blk_len);

    if (res != 0U)
    {
        g_usb_storage_state |= USB_STORAGE_WRITE_ERR;
    }

    return (int8_t)res;
}

static int8_t STORAGE_GetMaxLun(void)
{
    return (int8_t)(STORAGE_LUN_NBR - 1U);
}
