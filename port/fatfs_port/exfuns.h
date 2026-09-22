/**
 * @file    exfuns.h
 * @brief   FatFs application glue: per-volume FATFS objects and free-space
 *          query, ported from the vendor exfuns middleware. Uses a static
 *          FATFS pool so no heap is required.
 */

#ifndef PORT_EXFUNS_H
#define PORT_EXFUNS_H

#include <stdint.h>
#include "ff.h"

extern FATFS *fs[FF_VOLUMES];

/** @brief  Bind the static FATFS objects to the logical drives. */
uint8_t exfuns_init(void);

/**
 * @brief  Get total and free volume space.
 * @param  pdrv  drive string, e.g. "0:"
 * @param  total output, total size in KB
 * @param  free  output, free size in KB
 * @return FRESULT value (0 = FR_OK)
 */
uint8_t exfuns_get_free(uint8_t *pdrv, uint32_t *total, uint32_t *free);

#endif /* PORT_EXFUNS_H */
