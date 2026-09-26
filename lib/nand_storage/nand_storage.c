/**
 * @file    nand_storage.c
 * @brief   Strong NAND-sector glue over the FTL. The port layer keeps weak
 *          stubs for the FatFs NAND drive and the USB-MSC NAND logical unit;
 *          linking this object library (which the port may not depend on)
 *          overrides them, so only the apps that link it expose NAND storage.
 */

#include <stdint.h>
#include "ff.h"
#include "diskio.h"
#include "ftl.h"
#include "nand.h"
#include "nand_storage.h"

#define NAND_SECTOR_SIZE  512U

void nand_storage_activate(void)
{
    /* Referencing this symbol pulls this object (and its strong hooks) out of
     * the archive, so the port-layer weak stubs are overridden. */
}

/* ---- FatFs NAND drive hooks (ports to port/.../fatfs/diskio.c weak stubs) ---- */

DSTATUS nand_disk_status(void)
{
    return 0;
}

DSTATUS nand_disk_initialize(void)
{
    return (ftl_init() == 0U) ? 0 : STA_NOINIT;
}

DRESULT nand_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    if (ftl_read_sectors(buff, (uint32_t)sector, NAND_SECTOR_SIZE, count) != 0U)
    {
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT nand_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    if (ftl_write_sectors((uint8_t *)buff, (uint32_t)sector, NAND_SECTOR_SIZE, count) != 0U)
    {
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT nand_disk_ioctl(BYTE cmd, void *buff)
{
    switch (cmd)
    {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(DWORD *)buff = NAND_SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = (DWORD)(nand_dev.page_mainsize / NAND_SECTOR_SIZE);
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD *)buff = (DWORD)((uint32_t)nand_dev.valid_blocknum *
                                     nand_dev.block_pagenum *
                                     nand_dev.page_mainsize / NAND_SECTOR_SIZE);
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

/* ---- USB MSC LUN hooks (ports to port/.../usbd_storage_if.c weak stubs) ---- */

int8_t nand_storage_init(void)
{
    return (ftl_init() == 0U) ? 0 : -1;
}

int8_t nand_storage_capacity(uint32_t *block_num, uint16_t *block_size)
{
    *block_size = NAND_SECTOR_SIZE;
    *block_num  = (uint32_t)nand_dev.valid_blocknum *
                  nand_dev.block_pagenum *
                  nand_dev.page_mainsize / NAND_SECTOR_SIZE;
    return 0;
}

int8_t nand_storage_read(uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    return (ftl_read_sectors(buf, blk_addr, NAND_SECTOR_SIZE, blk_len) == 0U) ? 0 : -1;
}

int8_t nand_storage_write(uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    return (ftl_write_sectors(buf, blk_addr, NAND_SECTOR_SIZE, blk_len) == 0U) ? 0 : -1;
}
