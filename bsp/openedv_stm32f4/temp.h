/**
 * @file    temp.h
 * @brief   Temperature sensor: device-independent API over the chip driver
 *          (temp_ds18b20.h).
 */

#ifndef BSP_TEMP_H
#define BSP_TEMP_H

#include <stdint.h>

/** @brief  Reset the bus and probe the sensor. @return 0 if present. */
uint8_t temp_init(void);

/** @brief  Start a conversion and return the temperature in tenths of a degree. */
int16_t temp_read(void);

#endif /* BSP_TEMP_H */
