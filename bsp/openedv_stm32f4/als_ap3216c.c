/**
 * @file    als_ap3216c.c
 * @brief   AP3216C ambient light / proximity sensor driver.
 */

#include <stddef.h>

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "als_ap3216c.h"
#include "delay.h"

#define ALS_AP3216C_RESET_DELAY_MS  50U
#define ALS_AP3216C_DATA_LEN_BYTE        6U

/* Register map. */
#define ALS_AP3216C_SYS_REG     0x00U
#define ALS_AP3216C_DATA_REG    0x0AU
#define ALS_AP3216C_RESET       0x04U   /* software reset       */
#define ALS_AP3216C_ALS_PS_IR   0x03U   /* enable ALS + PS + IR */

static void ap3216c_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = data;
    (void)i2c_write(I2C_DEV_ALS, buf, 2U);
}

static uint8_t ap3216c_read_reg(uint8_t reg)
{
    uint8_t res = 0U;

    (void)i2c_write_read(I2C_DEV_ALS, &reg, 1U, &res, 1U);
    return res;
}

uint8_t als_ap3216c_init(void)
{
    uint8_t temp;

    i2c_init(NULL);

    ap3216c_write_reg(ALS_AP3216C_SYS_REG, ALS_AP3216C_RESET);
    delay_ms(ALS_AP3216C_RESET_DELAY_MS);
    ap3216c_write_reg(ALS_AP3216C_SYS_REG, ALS_AP3216C_ALS_PS_IR);

    temp = ap3216c_read_reg(ALS_AP3216C_SYS_REG);

    return (temp == ALS_AP3216C_ALS_PS_IR) ? 0U : 1U;
}

void als_ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als)
{
    uint8_t buf[ALS_AP3216C_DATA_LEN_BYTE];
    uint8_t i;

    for (i = 0U; i < ALS_AP3216C_DATA_LEN_BYTE; i++)
    {
        buf[i] = ap3216c_read_reg((uint8_t)(ALS_AP3216C_DATA_REG + i));
    }

    if ((buf[0] & 0x80U) != 0U)
    {
        *ir = 0U;   /* IR_OF set: IR reading invalid */
    }
    else
    {
        *ir = (uint16_t)(((uint16_t)buf[1] << 2) | (buf[0] & 0x03U));
    }

    *als = (uint16_t)(((uint16_t)buf[3] << 8) | buf[2]);

    if ((buf[4] & 0x40U) != 0U)
    {
        *ps = 0U;   /* PS data invalid */
    }
    else
    {
        *ps = (uint16_t)(((uint16_t)(buf[5] & 0x3FU) << 4) | (buf[4] & 0x0FU));
    }
}
