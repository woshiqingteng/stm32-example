/**
 * @file    spa06.c
 * @brief   SPA06 barometric pressure / temperature sensor driver.
 *
 * The compensation coefficients are read once during init() and the sensor is
 * left running in continuous mode. Altitude conversion is intentionally not
 * provided so the driver needs no libm dependency.
 */

#include "stm32f4xx_hal.h"
#include "iic.h"
#include "spa06.h"
#include "delay.h"

#define SPA06_STARTUP_DELAY_MS   40U
#define SPA06_SIGN_BIT_12        0x0800
#define SPA06_SIGN_MASK_12       0xF000
#define SPA06_SIGN_BIT_20        0x080000
#define SPA06_SIGN_MASK_20       0xFFF00000
#define SPA06_INT_FIFO_P_CFG     0x04U
#define SPA06_INT_FIFO_T_CFG     0x08U

typedef struct
{
    int16_t c0;
    int16_t c1;
    int32_t c00;
    int32_t c10;
    int16_t c01;
    int16_t c11;
    int16_t c20;
    int16_t c21;
    int16_t c30;
} spa06_calibcoeff_t;

static spa06_calibcoeff_t g_spa06_calib;
static int32_t g_spa06_kp;
static int32_t g_spa06_kt;

static const uint32_t g_spa06_scale_factor[8] =
{
    524288U, 1572864U, 3670016U, 7864320U, 253952U, 516096U, 1040384U, 2088960U
};

uint8_t spa06_write_byte(uint8_t reg, uint8_t data)
{
    iic_start();
    iic_send_byte((uint8_t)((SPA06_I2C_ADDR << 1) | 0x00U));

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    iic_send_byte(reg);

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    iic_send_byte(data);

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    iic_stop();
    return 0U;
}

uint8_t spa06_read_byte(uint8_t reg)
{
    uint8_t temp;

    iic_start();
    iic_send_byte((uint8_t)((SPA06_I2C_ADDR << 1) | 0x00U));

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 0U;
    }

    iic_send_byte(reg);

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 0U;
    }

    iic_start();
    iic_send_byte((uint8_t)((SPA06_I2C_ADDR << 1) | 0x01U));

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 0U;
    }

    temp = iic_read_byte(0);
    iic_stop();

    return temp;
}

static uint8_t spa06_read_nbytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    iic_start();
    iic_send_byte((uint8_t)((SPA06_I2C_ADDR << 1) | 0x00U));

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    iic_send_byte(reg);

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    iic_start();
    iic_send_byte((uint8_t)((SPA06_I2C_ADDR << 1) | 0x01U));

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 1U;
    }

    for (i = 0U; i < len; i++)
    {
        buf[i] = iic_read_byte((i == (uint8_t)(len - 1U)) ? 0U : 1U);
    }

    iic_stop();
    return 0U;
}

static void spa06_get_calib_param(void)
{
    uint8_t buffer[SPA06_CALIB_COEFFICIENT_LENGTH] = {0};
    int16_t tmp16;
    int32_t tmp32;

    (void)spa06_read_nbytes(SPA06_COEFFICIENT_CALIB_REG, buffer, SPA06_CALIB_COEFFICIENT_LENGTH);

    tmp16 = (int16_t)(((int16_t)buffer[0] << 4) | (buffer[1] >> 4));
    g_spa06_calib.c0 = ((tmp16 & SPA06_SIGN_BIT_12) != 0) ? (int16_t)(tmp16 | SPA06_SIGN_MASK_12) : tmp16;

    tmp16 = (int16_t)(((int16_t)(buffer[1] & 0x0FU) << 8) | buffer[2]);
    g_spa06_calib.c1 = ((tmp16 & SPA06_SIGN_BIT_12) != 0) ? (int16_t)(tmp16 | SPA06_SIGN_MASK_12) : tmp16;

    tmp32 = ((int32_t)buffer[3] << 12) | ((int32_t)buffer[4] << 4) | ((int32_t)buffer[5] >> 4);
    g_spa06_calib.c00 = ((tmp32 & SPA06_SIGN_BIT_20) != 0) ? (tmp32 | SPA06_SIGN_MASK_20) : tmp32;

    tmp32 = ((int32_t)(buffer[5] & 0x0FU) << 16) | ((int32_t)buffer[6] << 8) | (int32_t)buffer[7];
    g_spa06_calib.c10 = ((tmp32 & SPA06_SIGN_BIT_20) != 0) ? (tmp32 | SPA06_SIGN_MASK_20) : tmp32;

    g_spa06_calib.c01 = (int16_t)(((int16_t)buffer[8] << 8) | buffer[9]);
    g_spa06_calib.c11 = (int16_t)(((int16_t)buffer[10] << 8) | buffer[11]);
    g_spa06_calib.c20 = (int16_t)(((int16_t)buffer[12] << 8) | buffer[13]);
    g_spa06_calib.c21 = (int16_t)(((int16_t)buffer[14] << 8) | buffer[15]);
    g_spa06_calib.c30 = (int16_t)(((int16_t)buffer[16] << 8) | buffer[17]);
}

static void spa06_rateset(uint8_t is_pressure, uint8_t measure_rate, uint8_t oversample_rate)
{
    uint8_t reg;

    if (is_pressure != 0U)
    {
        g_spa06_kp = (int32_t)g_spa06_scale_factor[oversample_rate];
        (void)spa06_write_byte(SPA06_PRESSURE_CFG_REG,
                               (uint8_t)((measure_rate << 4) | oversample_rate));

        if (oversample_rate > SPA06_OVERSAMP_8)
        {
            (void)spa06_read_nbytes(SPA06_INT_FIFO_CFG_REG, &reg, 1U);
            (void)spa06_write_byte(SPA06_INT_FIFO_CFG_REG, (uint8_t)(reg | SPA06_INT_FIFO_P_CFG));
        }
    }
    else
    {
        g_spa06_kt = (int32_t)g_spa06_scale_factor[oversample_rate];
        (void)spa06_write_byte(SPA06_TEMPERATURE_CFG_REG,
                               (uint8_t)((measure_rate << 4) | oversample_rate | 0x80U));

        if (oversample_rate > SPA06_OVERSAMP_8)
        {
            (void)spa06_read_nbytes(SPA06_INT_FIFO_CFG_REG, &reg, 1U);
            (void)spa06_write_byte(SPA06_INT_FIFO_CFG_REG, (uint8_t)(reg | SPA06_INT_FIFO_T_CFG));
        }
    }
}

static float spa06_get_temperature(int32_t temperature)
{
    float tsc = temperature / (float)g_spa06_kt;

    return (g_spa06_calib.c0 * 0.5f) + (g_spa06_calib.c1 * tsc);
}

static float spa06_get_pressure(int32_t pressure, int32_t temperature)
{
    float tsc = temperature / (float)g_spa06_kt;
    float psc = pressure / (float)g_spa06_kp;
    float qua2 = g_spa06_calib.c10 + (psc * (g_spa06_calib.c20 + (psc * g_spa06_calib.c30)));
    float qua3 = tsc * psc * (g_spa06_calib.c11 + (psc * g_spa06_calib.c21));

    return g_spa06_calib.c00 + (psc * qua2) + (tsc * g_spa06_calib.c01) + qua3;
}

void spa06_get_data(spa06_result_t *p_res)
{
    uint8_t data[SPA06_DATA_FRAME_SIZE];
    int32_t tmp;

    (void)spa06_read_nbytes(SPA06_PRESSURE_MSB_REG, data, SPA06_DATA_FRAME_SIZE);

    tmp = ((int32_t)data[0] << 16) | ((int32_t)data[1] << 8) | (int32_t)data[2];
    p_res->praw = ((tmp & 0x800000) != 0) ? (int32_t)(0xFF000000U | (uint32_t)tmp) : tmp;

    tmp = ((int32_t)data[3] << 16) | ((int32_t)data[4] << 8) | (int32_t)data[5];
    p_res->traw = ((tmp & 0x800000) != 0) ? (int32_t)(0xFF000000U | (uint32_t)tmp) : tmp;

    p_res->tcomp = spa06_get_temperature(p_res->traw);
    p_res->pcomp = spa06_get_pressure(p_res->praw, p_res->traw) / 100.0f;
}

uint8_t spa06_init(void)
{
    uint8_t chip_id;

    iic_init();
    delay_ms(SPA06_STARTUP_DELAY_MS);

    chip_id = spa06_read_byte(SPA06_CHIP_ID);

    if (chip_id != SPA06_DEFAULT_CHIP_ID)
    {
        return 1U;
    }

    spa06_get_calib_param();

    spa06_rateset(1U, SPA06_MEASURE_16, SPA06_OVERSAMP_64);
    spa06_rateset(0U, SPA06_MEASURE_16, SPA06_OVERSAMP_64);

    return spa06_write_byte(SPA06_MODE_CFG_REG, SPA06_CONTINUOUS_MODE);
}
