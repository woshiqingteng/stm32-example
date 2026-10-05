/**
 * @file    fusion.h
 * @brief   Attitude fusion: 6-axis Madgwick AHRS and IMU bias calibration.
 */

#ifndef FUSION_H
#define FUSION_H

#include <stdint.h>

#define DEG2RAD   0.017453293f    /* pi / 180 */
#define RAD2DEG   57.29578f       /* 180 / pi */

/**
 * @brief  Zero-bias calibration. Keep the board still and level while it runs:
 *         the gyro bias is the averaged rate and the acc bias keeps |acc| = 1g
 *         along the measured gravity direction.
 */
void fusion_calibrate(void);

/** @brief  Read the bias-corrected accelerometer/gyroscope counts. */
void fusion_read_xyz(int16_t acc[3], int16_t gyro[3]);

/** @brief  Slowly track the gyro bias while the board is stationary (call every
 *          sample with the bias-corrected values from fusion_read_xyz()). */
void fusion_update_dynamic_bias(const int16_t acc[3], const int16_t gyro[3]);

/**
 * @brief  Update the attitude and return ZYX Euler angles in degrees:
 *         rpy[0] = pitch, rpy[1] = roll, rpy[2] = yaw.
 * @param  acc  accelerometer vector in g (not modified)
 * @param  gyro gyroscope vector in degrees per second (not modified)
 * @param  rpy  output angles in degrees
 * @param  dt   update period in seconds
 */
void fusion_get_eulerian_angles(const float acc[3], const float gyro[3], float *rpy, float dt);

#endif /* FUSION_H */
