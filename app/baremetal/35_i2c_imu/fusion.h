/**
 * @file    fusion.h
 * @brief   Attitude fusion: 6-axis Madgwick AHRS.
 */

#ifndef FUSION_H
#define FUSION_H

#include <stdint.h>

#define DEG2RAD   0.017453293f    /* pi / 180 */
#define RAD2DEG   57.29578f       /* 180 / pi */

/* Board orientation: map the raw sensor axes to the fusion (body) axes. Each
 * fusion axis selects a raw axis (0=X, 1=Y, 2=Z) and a sign (+1/-1); applies to
 * both the accelerometer and the gyroscope.
 *
 * Raw SH3001 axes on this board (board laid flat, face up):
 *   +X = across the board (from the IIC edge to the opposite edge),
 *   +Y = along the long edge, +Z = out of the top face (up).
 * Resulting body frame (NED, +Z down): +X = across (IIC edge to opposite edge),
 * +Y = along the long edge (reversed), +Z = down. */
#define FUSION_REMAP_X_AXIS   0
#define FUSION_REMAP_X_SIGN   (+1)
#define FUSION_REMAP_Y_AXIS   1
#define FUSION_REMAP_Y_SIGN   (-1)
#define FUSION_REMAP_Z_AXIS   2
#define FUSION_REMAP_Z_SIGN   (-1)

/* Body axis that points up at the start-up calibration pose (board flat, face
 * up). The accelerometer reads +1 g along it, so the calibration applies the
 * 1 g reference on this axis. Axis: 0=X, 1=Y, 2=Z. Sign: +1 if the body axis
 * points up, -1 if it points down. The body +Z points down, so up is -Z. */
#define FUSION_UP_AXIS        2
#define FUSION_UP_SIGN        (-1)

/* Euler output signs (degrees): rpy[0] = pitch, rpy[1] = roll, rpy[2] = yaw. */
#define FUSION_PITCH_SIGN     (+1)
#define FUSION_ROLL_SIGN      (+1)
#define FUSION_YAW_SIGN       (+1)

/**
 * @brief  Start-up zero-bias calibration. Called once at boot with the board
 *         still and level: 100 samples are averaged; the accelerometer Z offset
 *         is measured relative to 1g.
 */
void fusion_calibrate(void);

/** @brief  Read the accelerometer/gyroscope counts (bias/offset corrected). */
void fusion_read_xyz(int16_t acc[3], int16_t gyro[3]);

/** @brief  Die temperature (degC) from the latest fusion_read_xyz() sample. */
float fusion_get_temperature(void);

/** @brief  Slowly track the gyro bias while the board is stationary (call every
 *          sample with the values from fusion_read_xyz()). */
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

/** @brief  Current orientation as a unit quaternion (w, x, y, z). */
void fusion_get_quaternion(float q[4]);

#endif /* FUSION_H */
