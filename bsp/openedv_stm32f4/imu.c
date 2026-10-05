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

uint8_t imu_read(imu_data_t *data)
{
    return imu_sh3001_read(data->acc, data->gyro, &data->temperature);
}
