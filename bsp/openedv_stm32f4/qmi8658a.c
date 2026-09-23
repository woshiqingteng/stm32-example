/**
 * @file    qmi8658a.c
 * @brief   QMI8658A six-axis IMU driver.
 *
 * Only raw sensor access is provided: accelerometer and gyroscope counts plus
 * the die temperature. The accelerometer is configured for +/-8g and the
 * gyroscope for +/-512dps, both at 500Hz with the low-pass filter enabled.
 */

#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "iic.h"
#include "qmi8658a.h"
#include "delay.h"

#define QMI8658A_RESET_DELAY_MS     150U
#define QMI8658A_PROBE_RETRIES      5U
#define QMI8658A_DATA_LEN           12U
#define QMI8658A_TEMP_LEN           2U
#define QMI8658A_TEMP_DIVISOR       256.0f

/* CTRL2: accelerometer range/ODR, self-test enabled. */
#define QMI8658A_ACC_8G_500HZ_ST    0xA4U
/* CTRL3: gyroscope range/ODR, self-test enabled. */
#define QMI8658A_GYR_512DPS_500HZ_ST 0xD4U

static int16_t g_qmi8658a_acc[3];
static int16_t g_qmi8658a_gyro[3];

static uint8_t qmi8658a_write_byte(uint8_t reg, uint8_t data)
{
    iic_start();
    iic_send_byte((uint8_t)((QMI8658A_ADDR << 1) | 0x00U));

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

static uint8_t qmi8658a_read_byte(uint8_t reg)
{
    uint8_t temp;

    iic_start();
    iic_send_byte((uint8_t)((QMI8658A_ADDR << 1) | 0x00U));

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
    iic_send_byte((uint8_t)((QMI8658A_ADDR << 1) | 0x01U));

    if (iic_wait_ack() != 0U)
    {
        iic_stop();
        return 0U;
    }

    temp = iic_read_byte(0);
    iic_stop();

    return temp;
}

static uint8_t qmi8658a_read_nbytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    iic_start();
    iic_send_byte((uint8_t)((QMI8658A_ADDR << 1) | 0x00U));

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
    iic_send_byte((uint8_t)((QMI8658A_ADDR << 1) | 0x01U));

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

static uint8_t qmi8658a_check_whoami(void)
{
    uint8_t chip_id = 0;
    uint8_t revision_id = 0;
    uint8_t i;

    for (i = 0U; i < QMI8658A_PROBE_RETRIES; i++)
    {
        chip_id = qmi8658a_read_byte(QMI8658A_REG_WHO_AM_I);

        if (chip_id == QMI8658A_WHO_AM_I_VAL)
        {
            revision_id = qmi8658a_read_byte(QMI8658A_REG_REVISION);
            break;
        }
    }

    if ((chip_id == QMI8658A_WHO_AM_I_VAL) && (revision_id == QMI8658A_REVISION_VAL))
    {
        return 0U;
    }

    return 1U;
}

void qmi8658a_read_xyz(int16_t acc[3], int16_t gyro[3])
{
    uint8_t buf[QMI8658A_DATA_LEN];
    uint8_t status = 0;
    bool    ready = false;
    uint8_t retry;

    for (retry = 0U; retry < 3U; retry++)
    {
        status = qmi8658a_read_byte(QMI8658A_REG_STATUS0);

        if ((status & QMI8658A_STATUS_MASK) != 0U)
        {
            ready = true;
            break;
        }
    }

    if (ready)
    {
        (void)qmi8658a_read_nbytes(QMI8658A_REG_AX_L, buf, QMI8658A_DATA_LEN);

        g_qmi8658a_acc[0] = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);
        g_qmi8658a_acc[1] = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]);
        g_qmi8658a_acc[2] = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]);

        g_qmi8658a_gyro[0] = (int16_t)(((uint16_t)buf[7] << 8) | buf[6]);
        g_qmi8658a_gyro[1] = (int16_t)(((uint16_t)buf[9] << 8) | buf[8]);
        g_qmi8658a_gyro[2] = (int16_t)(((uint16_t)buf[11] << 8) | buf[10]);
    }

    acc[0] = g_qmi8658a_acc[0];
    acc[1] = g_qmi8658a_acc[1];
    acc[2] = g_qmi8658a_acc[2];

    gyro[0] = g_qmi8658a_gyro[0];
    gyro[1] = g_qmi8658a_gyro[1];
    gyro[2] = g_qmi8658a_gyro[2];
}

float qmi8658a_read_temperature(void)
{
    uint8_t buf[QMI8658A_TEMP_LEN];
    int16_t raw;

    (void)qmi8658a_read_nbytes(QMI8658A_REG_TEMP_L, buf, QMI8658A_TEMP_LEN);
    raw = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);

    return (float)raw / QMI8658A_TEMP_DIVISOR;
}

uint8_t qmi8658a_init(void)
{
    iic_init();

    (void)qmi8658a_write_byte(QMI8658A_REG_RESET, QMI8658A_RESET_CMD);
    delay_ms(QMI8658A_RESET_DELAY_MS);

    if (qmi8658a_check_whoami() != 0U)
    {
        return 1U;
    }

    (void)qmi8658a_write_byte(QMI8658A_REG_CTRL1, QMI8658A_CTRL1_VALUE);
    (void)qmi8658a_write_byte(QMI8658A_REG_CTRL7, 0x00U);
    (void)qmi8658a_write_byte(QMI8658A_REG_CTRL2, QMI8658A_ACC_8G_500HZ_ST);
    (void)qmi8658a_write_byte(QMI8658A_REG_CTRL3, QMI8658A_GYR_512DPS_500HZ_ST);
    (void)qmi8658a_write_byte(QMI8658A_REG_CTRL5, QMI8658A_CTRL5_VALUE);
    (void)qmi8658a_write_byte(QMI8658A_REG_CTRL7, QMI8658A_ENABLE_BOTH);

    return 0U;
}
