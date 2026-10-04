/**
 * @file    als.h
 * @brief   Ambient light / proximity sensor: IR, PS and ambient light.
 *          Device-independent API over the chip driver (als_ap3216c.h).
 */

#ifndef BSP_ALS_H
#define BSP_ALS_H

#include <stdint.h>

/** @brief  Reset and configure the sensor. @return 0 on success. */
uint8_t als_init(void);

/**
 * @brief  Read the raw IR, PS and ALS channels (raw ADC counts, not physical).
 * @param  ir   infrared channel (reflection / IR intensity)
 * @param  ps   proximity channel (object proximity)
 * @param  als  ambient-light channel (illumination)
 */
void als_read(uint16_t *ir, uint16_t *ps, uint16_t *als);

#endif /* BSP_ALS_H */
