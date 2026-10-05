/**
 * @file    imu.h
 * @brief   Six-axis IMU: raw accelerometer/gyroscope counts, temperature and
 *          motion interrupts. Device-independent API over the chip driver
 *          (imu_sh3001.h).
 */

#ifndef BSP_IMU_H
#define BSP_IMU_H

#include <stdint.h>

/* Motion interrupt status bits (returned by imu_motion_int_status()). */
#define IMU_STATUS_TAP           0x10U   /*!< [4] tap (single or double) */
#define IMU_STATUS_ACTIVITY      0x02U   /*!< [1] */
#define IMU_STATUS_FREEFALL      0x80U   /*!< [7] */

/** @brief  Probe and configure the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the chip id does not match. */
uint8_t imu_init(void);

/** @brief  Read raw accelerometer and gyroscope counts. */
void imu_read_raw(int16_t acc[3], int16_t gyro[3]);

/** @brief  Read the die temperature in degrees Celsius. */
float imu_read_temperature(void);

/** @brief  Configure and enable the tap, free-fall and activity interrupts
 *          (routed to the INT pin). */
void imu_motion_int_enable(void);

/** @brief  Read and clear the motion interrupt status. */
uint8_t imu_motion_int_status(void);

#endif /* BSP_IMU_H */
