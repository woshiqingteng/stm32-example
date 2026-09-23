/**
 * @file    mjpeg.h
 * @brief   Motion JPEG frame decoder: decodes one JPEG frame from memory and
 *          blits it to the RGB panel at a fixed offset.
 */

#ifndef LIB_MJPEG_MJPEG_H
#define LIB_MJPEG_MJPEG_H

#include <stdint.h>

/** @brief  Motion JPEG decoder result codes. */
typedef enum
{
    MJPEG_OK         = 0,   /*!< success */
    MJPEG_ERR_INIT   = 1,   /*!< decoder not initialised / no buffer */
    MJPEG_ERR_DECODE = 2,   /*!< LibJPEG reported a decode error */
    MJPEG_ERR_MEM    = 3,   /*!< scanline buffer allocation failed */
} mjpeg_status_t;

/** @brief  Prepare the decoder and its scanline buffer. @return MJPEG_OK on success. */
mjpeg_status_t mjpegdec_init(uint16_t offx, uint16_t offy);

/** @brief  Release the decoder buffers. */
void mjpegdec_free(void);

/** @brief  Decode one JPEG frame and draw it at the configured offset.
 *  @return MJPEG_OK on success. */
mjpeg_status_t mjpegdec_decode(uint8_t *buf, uint32_t bsize);

#endif /* LIB_MJPEG_MJPEG_H */
