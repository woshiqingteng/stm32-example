/**
 * @file    humi.c
 * @brief   Temperature / humidity sensor (delegates to the chip driver).
 */

#include "humi.h"
#include "humi_dht11.h"

uint8_t humi_init(void)
{
    return humi_dht11_init();
}

uint8_t humi_read(uint8_t *temp_c, uint8_t *rh)
{
    return humi_dht11_read_data(temp_c, rh);
}
