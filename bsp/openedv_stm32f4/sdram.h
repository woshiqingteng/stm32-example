/**
 * @file    sdram.h
 * @brief   On-board SDRAM (FMC bank5/6) driver.
 */

#ifndef BSP_SDRAM_H
#define BSP_SDRAM_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  SDRAM base address (FMC bank5). */
#define SDRAM_BASE_ADDR 0xC0000000UL

/** @brief  Initialise the SDRAM controller and memory. */
void sdram_init(void);

/** @brief  Copy len bytes from src into SDRAM at byte offset. */
void sdram_write_buffer(const uint8_t *src, uint32_t offset, uint32_t len);

/** @brief  Copy len bytes from SDRAM at byte offset into dst. */
void sdram_read_buffer(uint8_t *dst, uint32_t offset, uint32_t len);

#endif /* BSP_SDRAM_H */
