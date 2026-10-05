/**
 * @file    imu_sh3001.c
 * @brief   SH3001 six-axis IMU chip driver.
 *
 * Only raw sensor access is provided: accelerometer and gyroscope counts plus
 * the die temperature. The accelerometer is configured for +/-8g and the
 * gyroscope for +/-125dps, both at 125Hz.
 */

#include <stddef.h>

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "imu_sh3001.h"

#define IMU_SH3001_TEMP_DIVISOR   16.0f  /*!< TEMP DATA resolution, LSB per degC */
#define IMU_SH3001_TEMP_OFFSET_C  25.0f  /*!< reference temperature, degC */

static uint8_t imu_sh3001_write_byte(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = data;

    return i2c_write(I2C_DEV_IMU, buf, 2U) ? 0U : 1U;
}

static uint8_t imu_sh3001_read_byte(uint8_t reg)
{
    uint8_t data = 0U;

    (void)i2c_write_read(I2C_DEV_IMU, &reg, 1U, &data, 1U);
    return data;
}

static uint8_t imu_sh3001_read_nbytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_write_read(I2C_DEV_IMU, &reg, 1U, buf, (uint16_t)len) ? 0U : 1U;
}

static uint8_t imu_sh3001_check_chip_id(void)
{
    uint8_t chip_id = 0;
    uint8_t i;

    for (i = 0U; i < IMU_SH3001_PROBE_RETRY_COUNT; i++)
    {
        chip_id = imu_sh3001_read_byte(IMU_SH3001_REG_CHIP_ID);

        if (chip_id == IMU_SH3001_CHIP_ID_VAL)
        {
            break;
        }
    }

    return (chip_id == IMU_SH3001_CHIP_ID_VAL) ? 0U : 1U;
}

uint8_t imu_sh3001_init(void)
{
    uint8_t reg;

    i2c_init(NULL);

    if (imu_sh3001_check_chip_id() != 0U)
    {
        return 1U;
    }

    reg = imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG0);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TEMP_CONFIG0, (uint8_t)(reg | IMU_SH3001_TEMP_ENABLE));

    reg = imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG2);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TEMP_CONFIG2, (uint8_t)(reg & (uint8_t)~IMU_SH3001_TEMP_ANALOG_MASK));

    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACC_CONFIG1, IMU_SH3001_ACC_ODR_125HZ);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACC_CONFIG2, IMU_SH3001_ACC_RANGE_8G);
    reg = imu_sh3001_read_byte(IMU_SH3001_REG_ACC_CONFIG0);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACC_CONFIG0, (uint8_t)(reg | 0x01U));
    reg = imu_sh3001_read_byte(IMU_SH3001_REG_ACC_CONFIG3);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACC_CONFIG3,
                                (uint8_t)((reg & (uint8_t)~0xE0U) | IMU_SH3001_ACC_LPF_ODR_025));

    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG1, IMU_SH3001_GYRO_ODR_125HZ);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG3_X, IMU_SH3001_GYRO_RANGE_125DPS);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG3_Y, IMU_SH3001_GYRO_RANGE_125DPS);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG3_Z, IMU_SH3001_GYRO_RANGE_125DPS);
    reg = imu_sh3001_read_byte(IMU_SH3001_REG_GYRO_CONFIG2);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG2,
                                (uint8_t)((reg & (uint8_t)~0x0CU) | IMU_SH3001_GYRO_LPF_01));

    return 0U;
}

uint8_t imu_sh3001_read(int16_t acc[3], int16_t gyro[3], float *temperature)
{
    static uint16_t room = 0xFFFFU;   /* factory room offset, read from the chip once */
    uint8_t buf[IMU_SH3001_DATA_LEN_BYTE];
    uint8_t tbuf[IMU_SH3001_TEMP_LEN_BYTE];
    uint16_t temp;

    *temperature = 0.0f;

    if (imu_sh3001_read_nbytes(IMU_SH3001_REG_ACC_X_L, buf, IMU_SH3001_DATA_LEN_BYTE) != 0U)
    {
        return 1U;
    }

    acc[0]  = IMU_SH3001_MK16(buf[0], buf[1]);
    acc[1]  = IMU_SH3001_MK16(buf[2], buf[3]);
    acc[2]  = IMU_SH3001_MK16(buf[4], buf[5]);

    gyro[0] = IMU_SH3001_MK16(buf[6], buf[7]);
    gyro[1] = IMU_SH3001_MK16(buf[8], buf[9]);
    gyro[2] = IMU_SH3001_MK16(buf[10], buf[11]);

    if (imu_sh3001_read_nbytes(IMU_SH3001_REG_TEMP_L, tbuf, IMU_SH3001_TEMP_LEN_BYTE) == 0U)
    {
        if (room == 0xFFFFU)
        {
            room = (uint16_t)(((uint16_t)(imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG0) & 0x0FU) << 8) |
                              imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG1));
        }

        temp = (uint16_t)(((uint16_t)(tbuf[1] & 0x0FU) << 8) | tbuf[0]);
        *temperature = ((float)temp - (float)room) / IMU_SH3001_TEMP_DIVISOR + IMU_SH3001_TEMP_OFFSET_C;
    }

    return 0U;
}
