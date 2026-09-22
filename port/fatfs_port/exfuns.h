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

/* exfuns_file_type() result classes, taken from the high nibble. */
#define T_BIN       0x00
#define T_LRC       0x10
#define T_NES       0x20
#define T_SMS       0x21
#define T_TEXT      0x30
#define T_C         0x31
#define T_H         0x32
#define T_WAV       0x40
#define T_MP3       0x41
#define T_OGG       0x42
#define T_FLAC      0x43
#define T_AAC       0x44
#define T_WMA       0x45
#define T_MID       0x46
#define T_BMP       0x50
#define T_JPG       0x51
#define T_JPEG      0x52
#define T_GIF       0x53
#define T_AVI       0x60

/** @brief  Bind the static FATFS objects to the logical drives. */
uint8_t exfuns_init(void);

/** @brief  Classify a file name by extension (returns a T_* value, 0xFF for an
 *  unknown extension). */
uint8_t exfuns_file_type(char *fname);

/**
 * @brief  Get total and free volume space.
 * @param  pdrv  drive string, e.g. "0:"
 * @param  total output, total size in KB
 * @param  free  output, free size in KB
 * @return FRESULT value (0 = FR_OK)
 */
uint8_t exfuns_get_free(uint8_t *pdrv, uint32_t *total, uint32_t *free);

#endif /* PORT_EXFUNS_H */
