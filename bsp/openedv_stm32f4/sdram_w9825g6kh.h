/**
 * @file    sdram_w9825g6kh.h
 * @brief   Winbond W9825G6KH-6 SDRAM device parameters.
 *
 * 4 banks x 8192 rows x 512 columns x 16-bit = 32 MB, 8K refresh / 64 ms.
 * Timings from the -6 grade datasheet: absolute values in ns (tRCD 18, tRP 18,
 * tRC 60, tRAS 42, tXSR 72) and relative values in tCK (tMRD 2, tWR 2); the
 * FMC cycle counts are computed at run time from the actual SDCLK.
 * Included by sdram.c only.
 */

#ifndef BSP_SDRAM_W9825G6KH_H
#define BSP_SDRAM_W9825G6KH_H

#include "sdram.h"

static const sdram_cfg_t g_sdram_w9825g6kh =
{
    .col_bit    = FMC_SDRAM_COLUMN_BITS_NUM_9,   /* 512 columns  */
    .row_bit    = FMC_SDRAM_ROW_BITS_NUM_13,     /* 8192 rows    */
    .bank_num   = FMC_SDRAM_INTERN_BANKS_NUM_4,
    .data_width = FMC_SDRAM_MEM_BUS_WIDTH_16,
    .cas_latency = FMC_SDRAM_CAS_LATENCY_3,
    .sdclk_div  = 2U,                            /* SDCLK = HCLK / 2 */

    .tmrd_cycle = 2U,    /* 2 tCK */
    .twr_cycle  = 2U,    /* 2 tCK */
    .txsr_ns = 72U,
    .tras_ns = 42U,
    .trc_ns  = 60U,
    .trp_ns  = 18U,
    .trcd_ns = 18U,

    .refresh_period_ms = 64U,
    .row_num = 8192U,

    .mode_register = 0x0230U,   /* burst 1, sequential, CAS 3, write burst single */
};

#endif /* BSP_SDRAM_W9825G6KH_H */
