/**
 * @file    nand_mt29f4g08.h
 * @brief   Micron MT29F4G08ABADA (4 Gbit / 512 MB) NAND device parameters.
 *          Included by nand.c only.
 */

#ifndef BSP_NAND_MT29F4G08_H
#define BSP_NAND_MT29F4G08_H

#include <stdint.h>

/** @brief  On-board device ID (as returned by nand_readid()). */
#define NAND_MT29F4G08_ID   0xDC909556UL

/** @brief  Device geometry / layout descriptor. */
typedef struct
{
    uint32_t id;               /*!< device ID */
    uint16_t page_totalsize;   /*!< main + spare bytes per page */
    uint16_t page_mainsize;    /*!< main area bytes per page */
    uint16_t page_sparesize;   /*!< spare area bytes per page */
    uint8_t  block_pagenum;    /*!< pages per block */
    uint16_t plane_blocknum;   /*!< blocks per plane */
    uint16_t block_totalnum;   /*!< total blocks */
    uint16_t spare_ecc_offset; /*!< spare-area offset of the ECC bytes */
} nand_device_t;

/** @brief  Return the descriptor for @p id, or NULL if it is not this device. */
const nand_device_t *nand_mt29f4g08_probe(uint32_t id);

#endif /* BSP_NAND_MT29F4G08_H */
