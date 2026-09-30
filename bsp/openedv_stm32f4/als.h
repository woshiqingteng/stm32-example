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

/** @brief  Read the raw IR, proximity and ambient-light channels. */
void als_read(uint16_t *ir, uint16_t *ps, uint16_t *light);

#endif /* BSP_ALS_H */
