/**
 * @file    sdram.h
 * @brief   On-board SDRAM (FMC SDRAM bank1) interface.
 *
 * The FMC bring-up is device independent; the concrete device parameters
 * (geometry, timing, mode register) come from a chip header such as
 * sdram_w9825g6kh.h.
 *
 * Prerequisite: configure the system clock before calling sdram_init(); the
 * timing cycle counts and the refresh counter are derived from the current
 * HCLK at call time. Re-run sdram_init() if the clock changes afterwards.
 */

#ifndef BSP_SDRAM_H
#define BSP_SDRAM_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/** @brief  SDRAM base address (FMC SDRAM bank1). */
#define SDRAM_BASE_ADDR 0xC0000000UL

/** @brief  SDRAM device parameters (chip specific values, FMC generic type). */
typedef struct
{
    uint32_t col_bits;        /*!< FMC_SDRAM_COLUMN_BITS_NUM_x */
    uint32_t row_bits;        /*!< FMC_SDRAM_ROW_BITS_NUM_x */
    uint32_t bank_num;        /*!< FMC_SDRAM_INTERN_BANKS_NUM_x */
    uint32_t data_width;      /*!< FMC_SDRAM_MEM_BUS_WIDTH_x */
    uint32_t cas_latency;     /*!< FMC_SDRAM_CAS_LATENCY_x */
    uint32_t sdclk_div;       /*!< FMC SDCLK = HCLK / sdclk_div (2 or 3) */

    uint16_t tmrd_cycle;      /*!< load-to-active delay, in tCK */
    uint16_t twr_cycle;       /*!< write recovery time, in tCK */
    uint16_t txsr_ns;         /*!< exit self-refresh delay, ns */
    uint16_t tras_ns;         /*!< active-to-precharge (self-refresh) time, ns */
    uint16_t trc_ns;          /*!< row cycle delay, ns */
    uint16_t trp_ns;          /*!< row precharge delay, ns */
    uint16_t trcd_ns;         /*!< row-to-column delay, ns */

    uint16_t refresh_period_ms; /*!< tREF, device refresh period */
    uint16_t rows;              /*!< number of rows for the refresh counter */

    uint16_t mode_register;     /*!< SDRAM mode register value (CAS/burst) */
} sdram_cfg_t;

/** @brief  Initialise the FMC SDRAM controller and the device. */
void sdram_init(void);

/** @brief  Copy len bytes from src into SDRAM at byte offset. */
void sdram_write_buffer(const uint8_t *src, uint32_t offset, uint32_t len);

/** @brief  Copy len bytes from SDRAM at byte offset into dst. */
void sdram_read_buffer(uint8_t *dst, uint32_t offset, uint32_t len);

#endif /* BSP_SDRAM_H */
