/**
 * @file    imu.c
 * @brief   Attitude fusion for a 6-axis IMU, ported from the ALIENTEK
 *          QMI8658A experiment (imu.c). A Mahony-style complementary filter
 *          integrates the gyroscope and corrects with the accelerometer.
 */

#include <math.h>
#include "imu.h"

#define IMU_KP_INIT     20.0f   /* high gain while settling */
#define IMU_KP_NORMAL   1.0f
#define IMU_KI          0.01f
#define IMU_SETTLE_SAMPLE    200U    /* samples with the high gain */

static float q0 = 1.0f;
static float q1 = 0.0f;
static float q2 = 0.0f;
static float q3 = 0.0f;
static float rMat[3][3];

static float exInt = 0.0f;
static float eyInt = 0.0f;
static float ezInt = 0.0f;

static float imu_inv_sqrt(float x)
{
    float halfx = 0.5f * x;
    float y = x;
    long  i = *(long *)&y;

    i = 0x5f3759df - (i >> 1);
    y = *(float *)&i;
    y = y * (1.5f - (halfx * y * y));

    return y;
}

static void imu_computerotationmatrix(void)
{
    float q1q1 = q1 * q1;
    float q2q2 = q2 * q2;
    float q3q3 = q3 * q3;

    float q0q1 = q0 * q1;
    float q0q2 = q0 * q2;
    float q0q3 = q0 * q3;
    float q1q2 = q1 * q2;
    float q1q3 = q1 * q3;
    float q2q3 = q2 * q3;

    rMat[0][0] = 1.0f - 2.0f * q2q2 - 2.0f * q3q3;
    rMat[0][1] = 2.0f * (q1q2 + -q0q3);
    rMat[0][2] = 2.0f * (q1q3 - -q0q2);

    rMat[1][0] = 2.0f * (q1q2 - -q0q3);
    rMat[1][1] = 1.0f - 2.0f * q1q1 - 2.0f * q3q3;
    rMat[1][2] = 2.0f * (q2q3 + -q0q1);

    rMat[2][0] = 2.0f * (q1q3 + -q0q2);
    rMat[2][1] = 2.0f * (q2q3 - -q0q1);
    rMat[2][2] = 1.0f - 2.0f * q1q1 - 2.0f * q2q2;
}

void imu_get_eulerian_angles(float acc[3], float gyro[3], float *rpy, float dt)
{
    static unsigned short settle = IMU_SETTLE_SAMPLE;

    float normalise;
    float ex, ey, ez;
    float halfT = 0.5f * dt;
    float q0Last, q1Last, q2Last, q3Last;
    float kp;

    kp = (settle > 0U) ? (settle--, IMU_KP_INIT) : IMU_KP_NORMAL;

    if ((acc[0] != 0.0f) || (acc[1] != 0.0f) || (acc[2] != 0.0f))
    {
        normalise = imu_inv_sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
        acc[0] *= normalise;
        acc[1] *= normalise;
        acc[2] *= normalise;

        ex = (acc[1] * rMat[2][2] - acc[2] * rMat[2][1]);
        ey = (acc[2] * rMat[2][0] - acc[0] * rMat[2][2]);
        ez = (acc[0] * rMat[2][1] - acc[1] * rMat[2][0]);

        exInt += IMU_KI * ex * dt;
        eyInt += IMU_KI * ey * dt;
        ezInt += IMU_KI * ez * dt;

        gyro[0] += kp * ex + exInt;
        gyro[1] += kp * ey + eyInt;
        gyro[2] += kp * ez + ezInt;
    }

    q0Last = q0;
    q1Last = q1;
    q2Last = q2;
    q3Last = q3;
    q0 += (-q1Last * gyro[0] - q2Last * gyro[1] - q3Last * gyro[2]) * halfT;
    q1 += (q0Last * gyro[0] + q2Last * gyro[2] - q3Last * gyro[1]) * halfT;
    q2 += (q0Last * gyro[1] - q1Last * gyro[2] + q3Last * gyro[0]) * halfT;
    q3 += (q0Last * gyro[2] + q1Last * gyro[1] - q2Last * gyro[0]) * halfT;

    normalise = imu_inv_sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= normalise;
    q1 *= normalise;
    q2 *= normalise;
    q3 *= normalise;

    imu_computerotationmatrix();

    rpy[0] = asinf(rMat[2][0]) * RAD2DEG;
    rpy[1] = atan2f(rMat[2][1], rMat[2][2]) * RAD2DEG;
    rpy[2] = atan2f(rMat[1][0], rMat[0][0]) * RAD2DEG;
}
