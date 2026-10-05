/**
 * @file    mag.h
 * @brief   Magnetometer: 3-axis field and temperature. Device-independent API
 *          over the chip driver (mag_st480mc.h).
 */

#ifndef BSP_MAG_H
#define BSP_MAG_H

#include <stdint.h>

/** @brief  One single-shot sample. */
typedef struct {
    int16_t x;            /*!< magnetic field, X count */
    int16_t y;            /*!< magnetic field, Y count */
    int16_t z;            /*!< magnetic field, Z count */
    float   temperature;  /*!< die temperature, degrees Celsius */
} mag_data_t;

/** @brief  Reset and probe the magnetometer. @return 0 on success. */
uint8_t mag_init(void);

/** @brief  Read one sample (temperature + X/Y/Z) with a single measurement.
 *  @return 0 on success. */
uint8_t mag_read(mag_data_t *data);

#endif /* BSP_MAG_H */
