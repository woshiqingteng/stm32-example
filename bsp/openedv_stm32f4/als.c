/**
 * @file    als.c
 * @brief   Ambient light / proximity sensor (delegates to the chip driver).
 */

#include "als.h"
#include "als_ap3216c.h"

uint8_t als_init(void)
{
    return als_ap3216c_init();
}

void als_read(uint16_t *ir, uint16_t *ps, uint16_t *light)
{
    als_ap3216c_read_data(ir, ps, light);
}
