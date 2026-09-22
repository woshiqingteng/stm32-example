/**
 * @file    stmflash.h
 * @brief   Internal STM32F429 flash access: word/halfword program, sector erase
 *          and an EEPROM-style word block read/write over the last sector.
 */

#ifndef BSP_STMFLASH_H
#define BSP_STMFLASH_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define STMFLASH_BASE        0x08000000U /*!< start of the 1 MB internal flash */
#define STMFLASH_SIZE        0x100000U   /*!< STM32F429IG flash size */

/* Last 128 KB sector (sector 23) reserved for the EEPROM-style demo. */
#define STMFLASH_EEPROM_ADDR 0x081E0000U

/** @brief  Read one 32-bit word from flash. @param addr Must be 4-byte aligned. */
uint32_t stmflash_read_word(uint32_t addr);

/** @brief  Program one 32-bit word (target must be erased). */
void stmflash_write_word(uint32_t addr, uint32_t data);

/** @brief  Program one 16-bit half-word (target must be erased). */
void stmflash_write_halfword(uint32_t addr, uint16_t data);

/** @brief  Erase the single sector that contains @p addr. */
bool stmflash_erase_sector(uint32_t addr);

/** @brief  Copy @p words 32-bit words from flash to RAM. */
void stmflash_read(uint32_t addr, uint32_t *buf, uint32_t words);

/** @brief  Erase (when needed) and program @p words 32-bit words to flash. */
void stmflash_write(uint32_t addr, const uint32_t *buf, uint32_t words);

#endif /* BSP_STMFLASH_H */
