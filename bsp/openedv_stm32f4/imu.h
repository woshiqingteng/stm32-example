/**
 * @file    imu.h
 * @brief   Six-axis IMU: raw accelerometer/gyroscope counts and temperature.
 *          Device-independent API over the chip driver (imu_sh3001.h).
 */

#ifndef BSP_IMU_H
#define BSP_IMU_H

#include <stdint.h>

/** @brief  One IMU sample. */
typedef struct {
    int16_t acc[3];       /*!< accelerometer counts */
    int16_t gyro[3];      /*!< gyroscope counts */
    float   temperature;  /*!< die temperature, degrees Celsius */
} imu_data_t;

/** @brief  Probe and configure the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the chip id does not match. */
uint8_t imu_init(void);

/** @brief  Read one sample (accelerometer + gyroscope + temperature).
 *  @return 0 on success. */
uint8_t imu_read(imu_data_t *data);

#endif /* BSP_IMU_H */
