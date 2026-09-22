/**
 * @file    dcmi.h
 * @brief   DCMI camera interface with line-buffered DMA.
 *
 * The camera streams 8-bit parallel data over the DCMI bus. DMA2 stream 1 runs
 * in double-buffer mode with a pair of one-line ping-pong buffers; every
 * line-complete interrupt copies the finished line into the destination
 * framebuffer and invokes an optional line hook. A DCMI frame interrupt resets
 * the line index and invokes an optional frame hook.
 *
 * Pin map (ALIENTEK F429 camera header):
 *   D0..D7 : PC6 PC7 PC8 PC9 PC11 PD3 PB8 PB9
 *   PCLK   : PA6
 *   HREF   : PH8
 *   VSYNC  : PB7
 */

#ifndef BSP_DCMI_H
#define BSP_DCMI_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  Largest capture line the driver accepts, in pixels. */
#define DCMI_MAX_LINE_PIXELS 1024U

/** @brief  Called for every captured line, after it has been stored. */
typedef void (*dcmi_line_cb_t)(uint16_t *line, uint16_t index);

/** @brief  Called at the end of every captured frame. */
typedef void (*dcmi_frame_cb_t)(void);

/** @brief  Frames captured since dcmi_init() (incremented in the frame ISR). */
extern volatile uint32_t g_dcmi_frame_count;

/**
 * @brief  Initialise the DCMI interface and the DMA line engine.
 * @param  buf    Destination framebuffer (RGB565), may be the LTDC buffer.
 * @param  width  Framebuffer width in pixels (also the line stride).
 * @param  height Framebuffer height in pixels.
 */
void dcmi_init(uint16_t *buf, uint16_t width, uint16_t height);

/**
 * @brief  Place the captured image inside the framebuffer (letterbox / clip).
 * @param  x,y    Top-left corner inside the framebuffer.
 * @param  width  Captured width in pixels, must be even and <= DCMI_MAX_LINE_PIXELS.
 * @param  height Captured height in pixels.
 */
void dcmi_config(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/** @brief  Register (or clear with 0) the per-line hook. */
void dcmi_register_line_callback(dcmi_line_cb_t cb);

/** @brief  Register (or clear with 0) the per-frame hook. */
void dcmi_register_frame_callback(dcmi_frame_cb_t cb);

/** @brief  Start DMA + capture using the configured rectangle. */
void dcmi_start(void);

/** @brief  Stop capture and DMA. */
void dcmi_stop(void);

uint16_t dcmi_width(void);
uint16_t dcmi_height(void);

#endif /* BSP_DCMI_H */
