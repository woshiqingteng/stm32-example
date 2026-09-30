/**
 * @file    temp.c
 * @brief   Temperature sensor (delegates to the chip driver).
 */

#include "temp.h"
#include "ds18b20.h"

uint8_t temp_init(void)
{
    return ds18b20_init();
}

int16_t temp_read(void)
{
    return ds18b20_get_temperature();
}
