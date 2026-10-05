/**
 * @file    imu.c
 * @brief   Six-axis IMU (delegates to the chip driver).
 */

#include "imu.h"
#include "imu_sh3001.h"

uint8_t imu_init(void)
{
    return imu_sh3001_init();
}

void imu_read_raw(int16_t acc[3], int16_t gyro[3])
{
    (void)imu_sh3001_read_raw(acc, gyro);
}

float imu_read_temperature(void)
{
    return imu_sh3001_read_temperature();
}
