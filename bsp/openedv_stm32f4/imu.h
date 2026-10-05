/**
 * @file    imu.h
 * @brief   Six-axis IMU: raw accelerometer/gyroscope counts and temperature.
 *          Device-independent API over the chip driver (imu_sh3001.h).
 */

#ifndef BSP_IMU_H
#define BSP_IMU_H

#include <stdint.h>

/** @brief  Probe and configure the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the chip id does not match. */
uint8_t imu_init(void);

/** @brief  Read raw accelerometer and gyroscope counts. */
void imu_read_raw(int16_t acc[3], int16_t gyro[3]);

/** @brief  Read the die temperature in degrees Celsius. */
float imu_read_temperature(void);

#endif /* BSP_IMU_H */
