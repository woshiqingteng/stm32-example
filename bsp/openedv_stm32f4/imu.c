/**
 * @file    imu.c
 * @brief   Six-axis IMU: device-independent layer.
 *
 * Keeps the raw sensor state and the calibration / dynamic-bias logic; raw
 * access is delegated to the chip driver (imu_sh3001.h).
 */

#include <math.h>

#include "imu.h"
#include "imu_sh3001.h"
#include "delay.h"

static int16_t g_imu_acc[3];
static int16_t g_imu_gyro[3];
static float   g_imu_acc_bias[3];
static float   g_imu_gyro_bias[3];

uint8_t imu_init(void)
{
    return imu_sh3001_init();
}

void imu_calibrate(void)
{
    int32_t  sum_acc[3] = { 0, 0, 0 };
    int32_t  sum_gyro[3] = { 0, 0, 0 };
    int16_t  acc[3];
    int16_t  gyro[3];
    float    ax, ay, az;
    float    mag;
    float    scale;
    uint16_t i;

    for (i = 0U; i < IMU_CAL_SAMPLE_COUNT; i++)
    {
        imu_read_xyz(acc, gyro);

        sum_acc[0] += acc[0];
        sum_acc[1] += acc[1];
        sum_acc[2] += acc[2];
        sum_gyro[0] += gyro[0];
        sum_gyro[1] += gyro[1];
        sum_gyro[2] += gyro[2];

        delay_ms(IMU_CAL_SAMPLE_DELAY_MS);
    }

    g_imu_gyro_bias[0] = (float)sum_gyro[0] / (float)IMU_CAL_SAMPLE_COUNT;
    g_imu_gyro_bias[1] = (float)sum_gyro[1] / (float)IMU_CAL_SAMPLE_COUNT;
    g_imu_gyro_bias[2] = (float)sum_gyro[2] / (float)IMU_CAL_SAMPLE_COUNT;

    /* Keep the measured gravity vector at 1g (orientation independent). */
    ax = (float)sum_acc[0] / (float)IMU_CAL_SAMPLE_COUNT;
    ay = (float)sum_acc[1] / (float)IMU_CAL_SAMPLE_COUNT;
    az = (float)sum_acc[2] / (float)IMU_CAL_SAMPLE_COUNT;
    mag = sqrtf(ax * ax + ay * ay + az * az);

    if (mag > 1.0f)
    {
        scale = IMU_ACC_1G_COUNT / mag;
        g_imu_acc_bias[0] = ax * (1.0f - scale);
        g_imu_acc_bias[1] = ay * (1.0f - scale);
        g_imu_acc_bias[2] = az * (1.0f - scale);
    }
    else
    {
        g_imu_acc_bias[0] = 0.0f;
        g_imu_acc_bias[1] = 0.0f;
        g_imu_acc_bias[2] = 0.0f;
    }
}

void imu_read_xyz(int16_t acc[3], int16_t gyro[3])
{
    (void)imu_sh3001_read_raw(g_imu_acc, g_imu_gyro);

    acc[0] = (int16_t)((float)g_imu_acc[0] - g_imu_acc_bias[0]);
    acc[1] = (int16_t)((float)g_imu_acc[1] - g_imu_acc_bias[1]);
    acc[2] = (int16_t)((float)g_imu_acc[2] - g_imu_acc_bias[2]);

    gyro[0] = (int16_t)((float)g_imu_gyro[0] - g_imu_gyro_bias[0]);
    gyro[1] = (int16_t)((float)g_imu_gyro[1] - g_imu_gyro_bias[1]);
    gyro[2] = (int16_t)((float)g_imu_gyro[2] - g_imu_gyro_bias[2]);
}

void imu_update_dynamic_bias(const int16_t acc[3], const int16_t gyro[3])
{
    float   ax = (float)acc[0] / IMU_ACC_1G_COUNT;
    float   ay = (float)acc[1] / IMU_ACC_1G_COUNT;
    float   az = (float)acc[2] / IMU_ACC_1G_COUNT;
    float   mag = sqrtf(ax * ax + ay * ay + az * az);
    uint8_t i;

    if ((mag < IMU_DYN_ACC_MIN_G) || (mag > IMU_DYN_ACC_MAX_G))
    {
        return;
    }

    for (i = 0U; i < 3U; i++)
    {
        if (((gyro[i] < 0) ? -gyro[i] : gyro[i]) >= IMU_DYN_GYRO_THR_COUNT)
        {
            return;
        }
    }

    for (i = 0U; i < 3U; i++)
    {
        g_imu_gyro_bias[i] += IMU_DYN_BIAS_ALPHA * (float)gyro[i];
    }
}

float imu_read_temperature(void)
{
    return imu_sh3001_read_temperature();
}

void imu_motion_int_enable(void)
{
    imu_sh3001_motion_int_enable();
}

uint8_t imu_motion_int_status(void)
{
    return imu_sh3001_motion_int_status();
}

void imu_fifo_init(void)
{
    imu_sh3001_fifo_init();
}

uint16_t imu_fifo_level(void)
{
    return imu_sh3001_fifo_level();
}

uint8_t imu_fifo_read(uint8_t *buf, uint16_t len)
{
    return imu_sh3001_fifo_read(buf, len);
}
