/**
 * @file    nand_mt29f4g08.c
 * @brief   Micron MT29F4G08ABADA NAND device parameters (large page, 2 KB main
 *          + 64 B spare, 64 pages/block, 4096 blocks = 512 MB).
 */

#include "nand_mt29f4g08.h"

static const nand_device_t g_nand_mt29f4g08 =
{
    .id               = NAND_MT29F4G08_ID,
    .page_totalsize   = 2112U,
    .page_mainsize    = 2048U,
    .page_sparesize   = 64U,
    .block_pagenum    = 64U,
    .plane_blocknum   = 2048U,
    .block_totalnum   = 4096U,
    .spare_ecc_offset = 0x10U,
};

const nand_device_t *nand_mt29f4g08_probe(uint32_t id)
{
    return (id == g_nand_mt29f4g08.id) ? (const nand_device_t *)&g_nand_mt29f4g08 : 0;
}
