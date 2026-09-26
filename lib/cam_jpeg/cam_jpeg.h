/**
 * @file    cam_jpeg.h
 * @brief   One-shot JPEG frame capture over the DCMI into a caller buffer.
 */

#ifndef LIB_CAM_JPEG_H
#define LIB_CAM_JPEG_H

#include <stdint.h>
#include <stdbool.h>

/** @brief  Capture phase, polled by the application. */
typedef enum
{
    CAM_JPEG_IDLE = 0,   /*!< not capturing */
    CAM_JPEG_CAPTURING,  /*!< a frame is being received */
    CAM_JPEG_READY       /*!< a complete frame is in the buffer */
} cam_jpeg_state_t;

/**
 * @brief  Bind the destination buffer.
 * @param  dst        destination buffer (word aligned)
 * @param  max_words  capacity in 32-bit words
 */
void cam_jpeg_init(uint32_t *dst, uint32_t max_words);

/** @brief  Start a capture (configures the DCMI DMA and its callbacks). */
void cam_jpeg_begin(void);

/** @brief  Stop the DCMI; leaves a completed frame as CAM_JPEG_READY. */
void cam_jpeg_end(void);

/** @brief  Current capture phase. */
cam_jpeg_state_t cam_jpeg_state(void);

/** @brief  Captured frame size in 32-bit words. */
uint32_t cam_jpeg_words(void);

/** @brief  Convenience: begin, wait up to timeout_ms, then stop. */
bool cam_jpeg_capture(uint32_t timeout_ms);

#endif /* LIB_CAM_JPEG_H */
