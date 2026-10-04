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

uint8_t mag_read(int16_t *x, int16_t *y, int16_t *z)
{
    return mag_st480mc_read_magdata(x, y, z);
}

uint8_t mag_read_temp(float *temp)
{
    return mag_st480mc_read_temp(temp);
}
