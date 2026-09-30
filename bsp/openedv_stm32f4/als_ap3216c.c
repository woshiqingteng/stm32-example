/**
 * @file    als_ap3216c.c
 * @brief   AP3216C ambient light / proximity sensor driver.
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "als_ap3216c.h"
#include "delay.h"

#define ALS_AP3216C_RESET_DELAY_MS  50U
#define ALS_AP3216C_DATA_LEN_BYTE        6U

uint8_t als_ap3216c_write_one_byte(uint8_t reg, uint8_t data)
{
    i2c_start();
    i2c_send_byte(ALS_AP3216C_ADDR | 0x00U);

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_send_byte(reg);
    i2c_wait_ack();
    i2c_send_byte(data);

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_stop();
    return 0U;
}

uint8_t als_ap3216c_read_one_byte(uint8_t reg)
{
    uint8_t res;

    i2c_start();
    i2c_send_byte(ALS_AP3216C_ADDR | 0x00U);
    i2c_wait_ack();
    i2c_send_byte(reg);
    i2c_wait_ack();

    i2c_start();
    i2c_send_byte(ALS_AP3216C_ADDR | 0x01U);
    i2c_wait_ack();
    res = i2c_read_byte(0);
    i2c_stop();

    return res;
}

uint8_t als_ap3216c_init(void)
{
    uint8_t temp;

    i2c_init();

    als_ap3216c_write_one_byte(ALS_AP3216C_SYS_REG, ALS_AP3216C_RESET);
    delay_ms(ALS_AP3216C_RESET_DELAY_MS);
    als_ap3216c_write_one_byte(ALS_AP3216C_SYS_REG, ALS_AP3216C_ALS_PS_IR);

    temp = als_ap3216c_read_one_byte(ALS_AP3216C_SYS_REG);

    return (temp == ALS_AP3216C_ALS_PS_IR) ? 0U : 1U;
}

void als_ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als)
{
    uint8_t buf[ALS_AP3216C_DATA_LEN_BYTE];
    uint8_t i;

    for (i = 0U; i < ALS_AP3216C_DATA_LEN_BYTE; i++)
    {
        buf[i] = als_ap3216c_read_one_byte((uint8_t)(ALS_AP3216C_DATA_REG + i));
    }

    if ((buf[0] & 0x80U) != 0U)
    {
        *ir = 0U;
    }
    else
    {
        *ir = (uint16_t)(((uint16_t)buf[1] << 2) | (buf[0] & 0x03U));
    }

    *als = (uint16_t)(((uint16_t)buf[3] << 8) | buf[2]);

    if ((buf[4] & 0x40U) != 0U)
    {
        *ps = 0U;
    }
    else
    {
        *ps = (uint16_t)(((uint16_t)(buf[5] & 0x3FU) << 4) | (buf[4] & 0x0FU));
    }
}
