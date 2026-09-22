/**
 * @file    jpeg_dec.h
 * @brief   JPEG decode / encode helpers built on the LibJPEG module.
 */

#ifndef LIB_PICTURE_JPEG_DEC_H
#define LIB_PICTURE_JPEG_DEC_H

#include <stdint.h>

/** @brief  Decode a JPEG/JPG file and draw it inside the current piclib window.
 *  @return 0 on success, non-zero on failure. */
uint8_t jpg_decode(const char *filename, uint8_t fast);

/** @brief  Encode a rectangle of the current RGB panel into a JPEG file. */
uint8_t jpg_encode(const char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

#endif /* LIB_PICTURE_JPEG_DEC_H */
