/**
 * @file    humi.h
 * @brief   Temperature / humidity sensor: device-independent API over the chip
 *          driver (humi_dht11.h).
 */

#ifndef BSP_HUMI_H
#define BSP_HUMI_H

#include <stdint.h>

/** @brief  Reset the bus and probe the sensor. @return 0 if present. */
uint8_t humi_init(void);

/**
 * @brief  Read one measurement.
 * @param  temp_c temperature in degrees Celsius (0..50)
 * @param  rh     relative humidity in percent (20..90)
 * @return 0 on success, 1 on checksum/response failure
 */
uint8_t humi_read(uint8_t *temp_c, uint8_t *rh);

#endif /* BSP_HUMI_H */
