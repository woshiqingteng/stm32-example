/**
 * @file    mjpeg.h
 * @brief   Motion JPEG frame decoder: decodes one JPEG frame from memory and
 *          blits it to the RGB panel at a fixed offset.
 */

#ifndef LIB_MJPEG_MJPEG_H
#define LIB_MJPEG_MJPEG_H

#include <stdint.h>

/** @brief  Prepare the decoder and its scanline buffer. @return 0 on success. */
uint8_t mjpegdec_init(uint16_t offx, uint16_t offy);

/** @brief  Release the decoder buffers. */
void mjpegdec_free(void);

/** @brief  Decode one JPEG frame and draw it at the configured offset.
 *  @return 0 on success. */
uint8_t mjpegdec_decode(uint8_t *buf, uint32_t bsize);

#endif /* LIB_MJPEG_MJPEG_H */
