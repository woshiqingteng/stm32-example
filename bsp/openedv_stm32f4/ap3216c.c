/**
 * @file    ap3216c.c
 * @brief   AP3216C ambient light / proximity sensor driver.
 */

#include "stm32f4xx_hal.h"
#include "iic.h"
#include "ap3216c.h"
#include "delay.h"

#define AP3216C_RESET_DELAY_MS  50U
#define AP3216C_DATA_LEN        6U

uint8_t ap3216c_write_one_byte(uint8_t reg, uint8_t data)
{
    iic_start();
    iic_send_byte(AP3216C_ADDR | 0x00U);

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    iic_send_byte(reg);
    iic_wait_ack();
    iic_send_byte(data);

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    iic_stop();
    return 0U;
}

uint8_t ap3216c_read_one_byte(uint8_t reg)
{
    uint8_t res;

    iic_start();
    iic_send_byte(AP3216C_ADDR | 0x00U);
    iic_wait_ack();
    iic_send_byte(reg);
    iic_wait_ack();

    iic_start();
    iic_send_byte(AP3216C_ADDR | 0x01U);
    iic_wait_ack();
    res = iic_read_byte(0);
    iic_stop();

    return res;
}

uint8_t ap3216c_init(void)
{
    uint8_t temp;

    iic_init();

    ap3216c_write_one_byte(AP3216C_SYS_REG, AP3216C_RESET);
    delay_ms(AP3216C_RESET_DELAY_MS);
    ap3216c_write_one_byte(AP3216C_SYS_REG, AP3216C_ALS_PS_IR);

    temp = ap3216c_read_one_byte(AP3216C_SYS_REG);

    return (temp == AP3216C_ALS_PS_IR) ? 0U : 1U;
}

void ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als)
{
    uint8_t buf[AP3216C_DATA_LEN];
    uint8_t i;

    for (i = 0U; i < AP3216C_DATA_LEN; i++)
    {
        buf[i] = ap3216c_read_one_byte((uint8_t)(AP3216C_DATA_REG + i));
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
