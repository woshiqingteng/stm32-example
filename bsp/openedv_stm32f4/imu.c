/**
 * @file    imu.c
 * @brief   SH3001 six-axis IMU driver.
 *
 * Only raw sensor access is provided: accelerometer and gyroscope counts plus
 * the die temperature. The accelerometer is configured for +/-8g and the
 * gyroscope for +/-500dps, both at 500Hz.
 */

#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "imu.h"

#define IMU_TEMP_DIVISOR   16.0f  /*!< TEMP DATA resolution, LSB per degC */
#define IMU_TEMP_OFFSET_C  25.0f  /*!< reference temperature, degC */

static int16_t g_imu_acc[3];
static int16_t g_imu_gyro[3];

static uint8_t imu_write_byte(uint8_t reg, uint8_t data)
{
    i2c_start();
    i2c_send_byte((uint8_t)((IMU_ADDR << 1) | 0x00U));

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_send_byte(reg);

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_send_byte(data);

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_stop();
    return 0U;
}

static uint8_t imu_read_byte(uint8_t reg)
{
    uint8_t data;

    i2c_start();
    i2c_send_byte((uint8_t)((IMU_ADDR << 1) | 0x00U));

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 0U;
    }

    i2c_send_byte(reg);

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 0U;
    }

    i2c_start();
    i2c_send_byte((uint8_t)((IMU_ADDR << 1) | 0x01U));

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 0U;
    }

    data = i2c_read_byte(0);
    i2c_stop();

    return data;
}

static uint8_t imu_read_nbytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    i2c_start();
    i2c_send_byte((uint8_t)((IMU_ADDR << 1) | 0x00U));

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_send_byte(reg);

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    i2c_start();
    i2c_send_byte((uint8_t)((IMU_ADDR << 1) | 0x01U));

    if (i2c_wait_ack() != 0U)
    {
        i2c_stop();
        return 1U;
    }

    for (i = 0U; i < len; i++)
    {
        buf[i] = i2c_read_byte((i == (uint8_t)(len - 1U)) ? 0U : 1U);
    }

    i2c_stop();
    return 0U;
}

static uint8_t imu_check_chip_id(void)
{
    uint8_t chip_id = 0;
    uint8_t i;

    for (i = 0U; i < IMU_PROBE_RETRY_COUNT; i++)
    {
        chip_id = imu_read_byte(IMU_REG_CHIP_ID);

        if (chip_id == IMU_CHIP_ID_VAL)
        {
            break;
        }
    }

    return (chip_id == IMU_CHIP_ID_VAL) ? 0U : 1U;
}

uint8_t imu_init(void)
{
    uint8_t reg;

    i2c_init();

    if (imu_check_chip_id() != 0U)
    {
        return 1U;
    }

    /* Temperature: enable the digital sensor and keep the factory room-temp offset. */
    reg = imu_read_byte(IMU_REG_TEMP_CONFIG0);
    (void)imu_write_byte(IMU_REG_TEMP_CONFIG0, (uint8_t)(reg | IMU_TEMP_ENABLE));

    reg = imu_read_byte(IMU_REG_TEMP_CONFIG2);
    (void)imu_write_byte(IMU_REG_TEMP_CONFIG2, (uint8_t)(reg & (uint8_t)~IMU_TEMP_ANALOG_MASK));

    /* Accelerometer: 500Hz, +/-8g, digital filter enabled. */
    (void)imu_write_byte(IMU_REG_ACC_CONFIG1, IMU_ACC_ODR_500HZ);
    (void)imu_write_byte(IMU_REG_ACC_CONFIG2, IMU_ACC_RANGE_8G);
    reg = imu_read_byte(IMU_REG_ACC_CONFIG0);
    (void)imu_write_byte(IMU_REG_ACC_CONFIG0, (uint8_t)(reg | 0x01U));

    /* Gyroscope: 500Hz, +/-500dps, digital filter enabled. */
    (void)imu_write_byte(IMU_REG_GYRO_CONFIG1, IMU_GYRO_ODR_500HZ);
    (void)imu_write_byte(IMU_REG_GYRO_CONFIG3_X, IMU_GYRO_RANGE_500DPS);
    (void)imu_write_byte(IMU_REG_GYRO_CONFIG3_Y, IMU_GYRO_RANGE_500DPS);
    (void)imu_write_byte(IMU_REG_GYRO_CONFIG3_Z, IMU_GYRO_RANGE_500DPS);

    return 0U;
}

void imu_read_xyz(int16_t acc[3], int16_t gyro[3])
{
    uint8_t buf[IMU_DATA_LEN_BYTE];

    if (imu_read_nbytes(IMU_REG_ACC_X_L, buf, IMU_DATA_LEN_BYTE) == 0U)
    {
        g_imu_acc[0] = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);
        g_imu_acc[1] = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]);
        g_imu_acc[2] = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]);

        g_imu_gyro[0] = (int16_t)(((uint16_t)buf[7] << 8) | buf[6]);
        g_imu_gyro[1] = (int16_t)(((uint16_t)buf[9] << 8) | buf[8]);
        g_imu_gyro[2] = (int16_t)(((uint16_t)buf[11] << 8) | buf[10]);
    }

    acc[0] = g_imu_acc[0];
    acc[1] = g_imu_acc[1];
    acc[2] = g_imu_acc[2];

    gyro[0] = g_imu_gyro[0];
    gyro[1] = g_imu_gyro[1];
    gyro[2] = g_imu_gyro[2];
}

float imu_read_temperature(void)
{
    uint8_t buf[IMU_TEMP_LEN_BYTE];
    uint16_t temp;
    uint16_t room;

    if (imu_read_nbytes(IMU_REG_TEMP_L, buf, IMU_TEMP_LEN_BYTE) != 0U)
    {
        return 0.0f;
    }

    temp = (uint16_t)(((uint16_t)(buf[1] & 0x0FU) << 8) | buf[0]);
    room = (uint16_t)(((uint16_t)(imu_read_byte(IMU_REG_TEMP_CONFIG0) & 0x0FU) << 8) |
                      imu_read_byte(IMU_REG_TEMP_CONFIG1));

    return ((float)temp - (float)room) / IMU_TEMP_DIVISOR + IMU_TEMP_OFFSET_C;
}
