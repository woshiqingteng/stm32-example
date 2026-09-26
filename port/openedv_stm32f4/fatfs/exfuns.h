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

/** @brief  File type classes returned by exfuns_file_type(). */
typedef enum
{
    T_BIN     = 0x00,
    T_LRC     = 0x10,
    T_NES     = 0x20,
    T_SMS     = 0x21,
    T_TEXT    = 0x30,
    T_C       = 0x31,
    T_H       = 0x32,
    T_WAV     = 0x40,
    T_MP3     = 0x41,
    T_OGG     = 0x42,
    T_FLAC    = 0x43,
    T_AAC     = 0x44,
    T_WMA     = 0x45,
    T_MID     = 0x46,
    T_BMP     = 0x50,
    T_JPG     = 0x51,
    T_JPEG    = 0x52,
    T_GIF     = 0x53,
    T_AVI     = 0x60,
    T_UNKNOWN = 0xFF
} file_type_t;

/** @brief  Bind the static FATFS objects to the logical drives. */
uint8_t exfuns_init(void);

/** @brief  Classify a file name by extension (returns a file_type_t value,
 *  T_UNKNOWN for an unknown extension). */
file_type_t exfuns_file_type(char *fname);

/**
 * @brief  Get total and free volume space.
 * @param  pdrv  drive string, e.g. "0:"
 * @param  total output, total size in KB
 * @param  free  output, free size in KB
 * @return FRESULT value (0 = FR_OK)
 */
uint8_t exfuns_get_free(uint8_t *pdrv, uint32_t *total, uint32_t *free);

#endif /* PORT_EXFUNS_H */
