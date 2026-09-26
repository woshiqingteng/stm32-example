/**
 * @file    st480mc.c
 * @brief   ST480MC magnetometer driver, ported from the vendor example. Uses
 *          the shared i2c primitive API (SCL = PH4, SDA = PH5).
 */

#include "i2c.h"
#include "delay.h"
#include "st480mc.h"

uint8_t st480mc_read_nbytes(uint8_t addr, uint8_t length, uint8_t *buf)
{
    uint8_t i;

    i2c_start();
    i2c_send_byte((uint8_t)((ST480MC_ADDR << 1) | 0x00U));   /* address, write */

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_send_byte(addr);                                     /* register/command */
    (void)i2c_wait_ack();

    i2c_start();
    i2c_send_byte((uint8_t)((ST480MC_ADDR << 1) | 0x01U));   /* address, read */
    (void)i2c_wait_ack();

    for (i = 0U; i < length; i++)
    {
        buf[i] = i2c_read_byte(1U);
    }

    i2c_stop();

    return 0U;
}

uint8_t st480mc_write_register(uint8_t reg, uint16_t data)
{
    i2c_start();
    i2c_send_byte((uint8_t)((ST480MC_ADDR << 1) | 0x00U));

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_send_byte(ST480MC_WRITE_REG);
    (void)i2c_wait_ack();

    i2c_send_byte((uint8_t)(data >> 8));
    (void)i2c_wait_ack();
    i2c_send_byte((uint8_t)(data & 0xFFU));
    (void)i2c_wait_ack();

    i2c_send_byte((uint8_t)(reg << 2));                      /* address, low 2 bits 0 */
    (void)i2c_wait_ack();

    i2c_stop();

    return 0U;
}

uint16_t st480mc_read_register(uint8_t reg)
{
    uint8_t buf[3];
    uint8_t i;

    i2c_start();
    i2c_send_byte((uint8_t)((ST480MC_ADDR << 1) | 0x00U));
    (void)i2c_wait_ack();

    i2c_send_byte(ST480MC_READ_REG);
    (void)i2c_wait_ack();

    i2c_send_byte((uint8_t)(reg << 2));
    (void)i2c_wait_ack();

    i2c_start();
    i2c_send_byte((uint8_t)((ST480MC_ADDR << 1) | 0x01U));
    (void)i2c_wait_ack();

    for (i = 0U; i < 3U; i++)
    {
        buf[i] = i2c_read_byte(1U);
    }

    i2c_stop();

    if ((buf[0] & 0x10U) != 0U)
    {
        return 0xFFFFU;
    }

    return (uint16_t)(((uint16_t)buf[1] << 8) | buf[2]);
}

uint8_t st480mc_init(void)
{
    uint8_t status;
    uint8_t res    = 0xFFU;
    uint8_t retry  = 10U;

    i2c_init();

    /* Retry until the ST480MC answers with an ACK. */
    while ((retry-- != 0U) && (res != 0U))
    {
        res = st480mc_read_nbytes(ST480MC_RESET, 1U, &status);
        delay_ms(20U);
    }

    return res;
}

uint8_t st480mc_read_magdata(int16_t *pmagx, int16_t *pmagy, int16_t *pmagz)
{
    uint8_t buf[7];

    (void)st480mc_read_nbytes((uint8_t)(ST480MC_SINGLE_MODE & 0xFEU), 1U, buf); /* single-shot, no temp */
    delay_ms(15U);
    (void)st480mc_read_nbytes((uint8_t)(ST480MC_READ_DATA & 0xFEU), 7U, buf);   /* read mag data */

    if ((buf[0] & 0x10U) != 0U)
    {
        return buf[0];
    }

    *pmagx = (int16_t)(((int16_t)buf[1] << 8) | buf[2]);
    *pmagy = (int16_t)(((int16_t)buf[3] << 8) | buf[4]);
    *pmagz = (int16_t)(((int16_t)buf[5] << 8) | buf[6]);

    return 0U;
}

uint8_t st480mc_read_temperature(float *ptemp)
{
    uint8_t buf[9];
    int16_t raw;

    (void)st480mc_read_nbytes(ST480MC_SINGLE_MODE, 1U, buf);   /* single-shot, with temp */
    delay_ms(15U);
    (void)st480mc_read_nbytes(ST480MC_READ_DATA, 9U, buf);     /* read data + temp */

    if ((buf[0] & 0x10U) != 0U)
    {
        return buf[0];
    }

    raw   = (int16_t)(((uint16_t)buf[1] << 8) | buf[2]);
    *ptemp = ((float)raw - 46244.0f) / 45.2f + 25.0f;          /* vendor formula */

    return 0U;
}

uint8_t st480mc_read_magdata_average(int16_t *pmagx, int16_t *pmagy, int16_t *pmagz, uint8_t times)
{
    uint8_t i = 0U;
    uint8_t error_cnt = 0U;
    int32_t magx = 0;
    int32_t magy = 0;
    int32_t magz = 0;

    while (i < times)
    {
        if (st480mc_read_magdata(pmagx, pmagy, pmagz) == 0U)
        {
            magx += *pmagx;
            magy += *pmagy;
            magz += *pmagz;
            i++;
            error_cnt = 0U;
        }
        else
        {
            error_cnt++;
            delay_ms(10U);

            if (error_cnt > 100U)
            {
                return 0xFFU;
            }
        }
    }

    *pmagx = (int16_t)(magx / times);
    *pmagy = (int16_t)(magy / times);
    *pmagz = (int16_t)(magz / times);

    return 0U;
}
