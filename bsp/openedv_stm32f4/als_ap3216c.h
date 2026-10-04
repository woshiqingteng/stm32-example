/**
 * @file    als_ap3216c.h
 * @brief   AP3216C ambient light / proximity sensor driver.
 */

#ifndef BSP_ALS_AP3216C_H
#define BSP_ALS_AP3216C_H

#include <stdint.h>

/** @brief  Reset and configure the sensor.
 *  @return 0 on success, 1 if the system register did not read back. */
uint8_t als_ap3216c_init(void);

/**
 * @brief  Read the raw IR, PS and ALS channels (raw ADC counts, not physical).
 * @param  ir   infrared channel (reflection / IR intensity)
 * @param  ps   proximity channel (object proximity)
 * @param  als  ambient-light channel (illumination)
 */
void als_ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als);

#endif /* BSP_ALS_AP3216C_H */
