/**
 * @file    dcmi.h
 * @brief   DCMI camera interface driver.
 *
 * The camera streams 8-bit parallel data over the DCMI bus. DMA2 stream 1 runs
 * in circular (optionally double-buffered) mode; the transfer-complete hook is
 * invoked for every finished buffer and the application decides what to do with
 * it (copy a raw JPEG chunk into a big SDRAM buffer, or fill an LCD line). A
 * DCMI frame (VSYNC) hook is invoked at the end of every captured frame.
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

extern DCMI_HandleTypeDef g_dcmi_handle;    /*!< DCMI peripheral handle  */
extern DMA_HandleTypeDef  g_dma_dcmi_handle;/*!< DCMI DMA stream handle  */

/** @brief Hook called from the DMA transfer-complete interrupt; the just
 *         finished buffer is the one the DMA is not currently pointing at.
 *         Set to 0 to disable. */
extern void (*dcmi_rx_callback)(void);

/** @brief Hook called at the end of every captured frame (VSYNC). Set to 0 to
 *         disable. */
extern void (*dcmi_frame_callback)(void);

/** @brief Frames captured since dcmi_init() (incremented in the frame ISR). */
extern volatile uint32_t g_dcmi_frame_count;

/** @brief Initialise the DCMI interface and its pins (no DMA yet). */
void dcmi_init(void);

/**
 * @brief  Configure and start the DMA half/full transfer engine.
 * @param  mem0    First memory buffer address.
 * @param  mem1    Second memory buffer address, or 0 for single buffering.
 * @param  memsize Transfer size in memory items (per buffer).
 * @param  memblen Memory data alignment (DMA_MDATAALIGN_BYTE/HALFWORD/WORD).
 * @param  meminc  Memory increment mode (DMA_MINC_ENABLE/DISABLE).
 */
void dcmi_dma_init(uint32_t mem0, uint32_t mem1, uint16_t memsize, uint32_t memblen, uint32_t meminc);

/** @brief  Enable DMA + DCMI capture. */
void dcmi_start(void);

/** @brief  Stop capture and disable DMA. */
void dcmi_stop(void);

/** @brief  Re-configure the DCMI sampling polarities (debug / USMART). */
void dcmi_cr_set(uint8_t pclk, uint8_t hsync, uint8_t vsync);

/** @brief  Restart capture with a new sensor output window (debug / USMART). */
void dcmi_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);

#endif /* BSP_DCMI_H */
