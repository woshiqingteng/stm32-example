/**
 * @file    mag.c
 * @brief   Magnetometer (delegates to the chip driver).
 */

#include "mag.h"
#include "mag_st480mc.h"

uint8_t mag_init(void)
{
    return mag_st480mc_init();
}

uint8_t mag_read(mag_data_t *data)
{
    return mag_st480mc_read(&data->x, &data->y, &data->z, &data->temperature);
}
