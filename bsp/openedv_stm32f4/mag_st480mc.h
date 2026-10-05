/**
 * @file    mag_st480mc.h
 * @brief   ST480MC magnetometer (3-axis + temperature) over the shared IIC
 *          bus (SCL = PH4, SDA = PH5), ported from the vendor example.
 */

#ifndef BSP_MAG_ST480MC_H
#define BSP_MAG_ST480MC_H

#include <stdint.h>

/** @brief  Reset the ST480MC and probe its IIC address. 0 on success. */
uint8_t mag_st480mc_init(void);

/** @brief  One single measurement: temperature (Celsius) and X/Y/Z counts.
 *  @return 0 on success. */
uint8_t mag_st480mc_read(int16_t *x, int16_t *y, int16_t *z, float *temp);

#endif /* BSP_MAG_ST480MC_H */
