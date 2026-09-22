/**
 * @file    norflash.h
 * @brief   W25Qxx SPI NOR flash driver (SPI5, chip select PF6).
 */

#ifndef BSP_NORFLASH_H
#define BSP_NORFLASH_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define NORFLASH_CS_GPIO_PORT   GPIOF
#define NORFLASH_CS_GPIO_PIN    GPIO_PIN_6

/** @brief  Known device (JEDEC) IDs. */
#define W25Q80      0xEF13U
#define W25Q16      0xEF14U
#define W25Q32      0xEF15U
#define W25Q64      0xEF16U
#define W25Q128     0xEF17U
#define W25Q256     0xEF18U
#define BY25Q64     0x6816U
#define BY25Q128    0x6817U
#define BY25Q256    0x6818U
#define NM25Q64     0x5216U
#define NM25Q128    0x5217U

#define NORFLASH_SECTOR_SIZE    4096U
#define NORFLASH_PAGE_SIZE      256U

extern uint16_t g_norflash_type; /*!< detected device ID */

/** @brief  Bring up SPI5, probe the device and switch 256 Mbit parts to 4-byte addressing. */
void norflash_init(void);

/** @brief  Read the JEDEC manufacturer/device ID. */
uint16_t norflash_read_id(void);

/** @brief  Read @p datalen bytes starting at @p addr. */
void norflash_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase-on-demand write of @p datalen bytes starting at @p addr. */
void norflash_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase sector number @p saddr (each sector is 4 KB). */
void norflash_erase_sector(uint32_t saddr);

#endif /* BSP_NORFLASH_H */
