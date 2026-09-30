/**
 * @file    nor.h
 * @brief   SPI NOR flash storage: device-independent read/write/erase.
 *
 * The bus, chip select and JEDEC id handling live in the chip driver
 * (nor_w25q256jv.h); this layer holds the SPI-NOR command algorithms.
 */

#ifndef BSP_NOR_H
#define BSP_NOR_H

#include <stdint.h>

#define NOR_SECTOR_SIZE_BYTE    4096U
#define NOR_PAGE_SIZE_BYTE      256U

/** @brief  Bring up the SPI bus and probe the device. */
void nor_init(void);

/** @brief  Read the JEDEC manufacturer/device ID. */
uint16_t nor_read_id(void);

/** @brief  Read @p datalen bytes starting at @p addr. */
void nor_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase-on-demand write of @p datalen bytes starting at @p addr. */
void nor_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase sector number @p saddr (each sector is 4 KB). */
void nor_erase_sector(uint32_t saddr);

#endif /* BSP_NOR_H */
