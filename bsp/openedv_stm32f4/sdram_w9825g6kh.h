/**
 * @file    sdram_w9825g6kh.h
 * @brief   Winbond W9825G6KH-6 SDRAM device parameters.
 *
 * 4 banks x 8192 rows x 512 columns x 16-bit = 32 MB, 8K refresh / 64 ms.
 * Timings are SDCLK cycle counts at 90 MHz (tCK = 11.1 ns) taken from the
 * -6 grade datasheet:
 *   tRCD 18 ns, tRP 18 ns, tRC 60 ns, tRAS 42 ns, tWR 2 tCK, tXSR 72 ns.
 * Included by sdram.c only.
 */

#ifndef BSP_SDRAM_W9825G6KH_H
#define BSP_SDRAM_W9825G6KH_H

#include "sdram.h"

static const sdram_cfg_t g_sdram_w9825g6kh =
{
    .col_bits   = FMC_SDRAM_COLUMN_BITS_NUM_9,   /* 512 columns  */
    .row_bits   = FMC_SDRAM_ROW_BITS_NUM_13,     /* 8192 rows    */
    .bank_num   = FMC_SDRAM_INTERN_BANKS_NUM_4,
    .data_width = FMC_SDRAM_MEM_BUS_WIDTH_16,
    .cas_latency = FMC_SDRAM_CAS_LATENCY_3,
    .sdclk_div  = 2U,                            /* SDCLK = HCLK / 2 */

    .tmrd = 2U,          /* 2 tCK */
    .txsr = 7U,          /* > 72 ns @ 90 MHz */
    .tras = 4U,          /* > 42 ns @ 90 MHz */
    .trc  = 6U,          /* > 60 ns @ 90 MHz */
    .twr  = 2U,          /* 2 tCK */
    .trp  = 2U,          /* > 18 ns @ 90 MHz */
    .trcd = 2U,          /* > 18 ns @ 90 MHz */

    .refresh_period_ms = 64U,
    .rows = 8192U,

    .mode_register = 0x0230U,   /* burst 1, sequential, CAS 3, write burst single */
};

#endif /* BSP_SDRAM_W9825G6KH_H */
