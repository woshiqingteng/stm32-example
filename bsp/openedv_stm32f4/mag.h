/**
 * @file    mag.h
 * @brief   Magnetometer: 3-axis field and temperature. Device-independent API
 *          over the chip driver (mag_st480mc.h).
 */

#ifndef BSP_MAG_H
#define BSP_MAG_H

#include <stdint.h>

/** @brief  Reset and probe the magnetometer. @return 0 on success. */
uint8_t mag_init(void);

/** @brief  Read one raw magnetic sample (single-shot). @return 0 on success. */
uint8_t mag_read(int16_t *x, int16_t *y, int16_t *z);

/** @brief  Read the temperature in Celsius (single-shot). @return 0 on success. */
uint8_t mag_read_temp(float *temp);

#endif /* BSP_MAG_H */
