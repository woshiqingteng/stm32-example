/**
 * @file    text.h
 * @brief   GBK text rendering on the RGB panel (ALIENTEK TEXT middleware).
 */

#ifndef LIB_TEXT_TEXT_H
#define LIB_TEXT_TEXT_H

#include <stdint.h>
#include "fonts.h"

/** @brief  Draw one GBK glyph (2-byte code) at (x, y). */
void text_show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size, uint8_t mode, uint16_t color);

/** @brief  Draw a mixed ASCII/GBK string in the given window. */
void text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, uint8_t size, uint8_t mode, uint16_t color);

/** @brief  Draw a string horizontally centred inside width. */
void text_show_string_middle(uint16_t x, uint16_t y, char *str, uint8_t size, uint16_t width, uint16_t color);

#endif /* LIB_TEXT_TEXT_H */
