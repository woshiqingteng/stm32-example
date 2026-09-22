/**
 * @file    fonts.h
 * @brief   GBK font store metadata and update helpers, ported from the ALIENTEK
 *          TEXT middleware. The fonts live in the on-board SPI NOR flash.
 */

#ifndef LIB_TEXT_FONTS_H
#define LIB_TEXT_FONTS_H

#include <stdint.h>

/** @brief  Font store descriptor read from / written to the NOR flash. */
typedef struct __attribute__((packed))
{
    uint8_t  fontok;      /* 0xAA when the font store is valid */
    uint32_t ugbkaddr;    /* unigbk address */
    uint32_t ugbksize;    /* unigbk size */
    uint32_t f12addr;     /* gbk12 address */
    uint32_t gbk12size;   /* gbk12 size */
    uint32_t f16addr;     /* gbk16 address */
    uint32_t gbk16size;   /* gbk16 size */
    uint32_t f24addr;     /* gbk24 address */
    uint32_t gbk24size;   /* gbk24 size */
    uint32_t f32addr;     /* gbk32 address */
    uint32_t gbk32size;   /* gbk32 size */
} _font_info;

extern _font_info ftinfo;

/** @brief  Copy the whole GBK font set from a source drive ("0:" is the SD
 *  card) into the NOR flash. Returns 0 on success. */
uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color);

/** @brief  Load the font store descriptor. Returns 0 when it is valid. */
uint8_t fonts_init(void);

#endif /* LIB_TEXT_FONTS_H */
