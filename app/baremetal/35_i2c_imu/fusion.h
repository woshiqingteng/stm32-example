/**
 * @file    fusion.h
 * @brief   Attitude fusion (Mahony-style quaternion) for a 6-axis IMU.
 */

#ifndef FUSION_H
#define FUSION_H

#define FUSION_DELTA_T  0.015f          /* nominal update period, seconds */
#define DEG2RAD         0.017453293f    /* pi/180 */
#define RAD2DEG         57.29578f       /* 180/pi */

/**
 * @brief  Update the attitude estimate and return Euler angles.
 * @param  acc  accelerometer vector (any unit; it is normalised)
 * @param  gyro gyroscope vector in rad/s (updated in place)
 * @param  rpy  output roll/pitch/yaw in degrees
 * @param  dt   update period in seconds
 */
void fusion_get_eulerian_angles(float acc[3], float gyro[3], float *rpy, float dt);

#endif /* FUSION_H */
