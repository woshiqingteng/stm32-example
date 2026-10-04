/**
 * @file    imu_sh3001.c
 * @brief   SH3001 six-axis IMU chip driver.
 *
 * Only raw sensor access is provided: accelerometer and gyroscope counts plus
 * the die temperature. The accelerometer is configured for +/-8g and the
 * gyroscope for +/-500dps, both at 500Hz.
 */

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

    i2c_init(0);

    if (imu_sh3001_check_chip_id() != 0U)
    {
        return 1U;
    }

    /* Temperature: enable the digital sensor and keep the factory room-temp offset. */
    reg = imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG0);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TEMP_CONFIG0, (uint8_t)(reg | IMU_SH3001_TEMP_ENABLE));

    reg = imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG2);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TEMP_CONFIG2, (uint8_t)(reg & (uint8_t)~IMU_SH3001_TEMP_ANALOG_MASK));

    /* Accelerometer: 500Hz, +/-8g, digital filter enabled. */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACC_CONFIG1, IMU_SH3001_ACC_ODR_500HZ);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACC_CONFIG2, IMU_SH3001_ACC_RANGE_8G);
    reg = imu_sh3001_read_byte(IMU_SH3001_REG_ACC_CONFIG0);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACC_CONFIG0, (uint8_t)(reg | 0x01U));

    /* Gyroscope: 500Hz, +/-500dps, digital filter enabled. */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG1, IMU_SH3001_GYRO_ODR_500HZ);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG3_X, IMU_SH3001_GYRO_RANGE_500DPS);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG3_Y, IMU_SH3001_GYRO_RANGE_500DPS);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_GYRO_CONFIG3_Z, IMU_SH3001_GYRO_RANGE_500DPS);

    return 0U;
}

uint8_t imu_sh3001_read_raw(int16_t acc[3], int16_t gyro[3])
{
    uint8_t buf[IMU_SH3001_DATA_LEN_BYTE];

    if (imu_sh3001_read_nbytes(IMU_SH3001_REG_ACC_X_L, buf, IMU_SH3001_DATA_LEN_BYTE) != 0U)
    {
        return 1U;
    }

    acc[0] = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);
    acc[1] = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]);
    acc[2] = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]);

    gyro[0] = (int16_t)(((uint16_t)buf[7] << 8) | buf[6]);
    gyro[1] = (int16_t)(((uint16_t)buf[9] << 8) | buf[8]);
    gyro[2] = (int16_t)(((uint16_t)buf[11] << 8) | buf[10]);

    return 0U;
}

float imu_sh3001_read_temperature(void)
{
    uint8_t buf[IMU_SH3001_TEMP_LEN_BYTE];
    uint16_t temp;
    uint16_t room;

    if (imu_sh3001_read_nbytes(IMU_SH3001_REG_TEMP_L, buf, IMU_SH3001_TEMP_LEN_BYTE) != 0U)
    {
        return 0.0f;
    }

    temp = (uint16_t)(((uint16_t)(buf[1] & 0x0FU) << 8) | buf[0]);
    room = (uint16_t)(((uint16_t)(imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG0) & 0x0FU) << 8) |
                      imu_sh3001_read_byte(IMU_SH3001_REG_TEMP_CONFIG1));

    return ((float)temp - (float)room) / IMU_SH3001_TEMP_DIVISOR + IMU_SH3001_TEMP_OFFSET_C;
}

void imu_sh3001_motion_int_enable(void)
{
    /* Tap: X/Y/Z, |acc|/64 above 0x51 within 0x52 ms. */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TAP_AXIS, 0x0EU);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TAP_THR, 0x10U);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TAP_DUR, 0x50U);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TAP_LAT, 0x40U);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_TAP_WIN, 0x80U);

    /* Free-fall: ~300mg threshold at 2mg/LSB, 160ms (2ms/LSB). */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_FREEFALL_THR, 0x96U);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_FREEFALL_TIME, 0x50U);

    /* Activity: X/Y/Z in AC mode (delta of consecutive samples, 0.97mg/LSB at
     * +/-8g), ~31mg threshold over 3 samples. */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACT_AXIS, 0xF0U);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACT_THR, 0x20U);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_ACT_TIME, 0x03U);

    /* Interrupt config: latched, active high, normal output (cleared by
     * reading the interrupt status register). */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_INT_CONFIG, 0x05U);

    /* The SH3001 keeps its configuration across an MCU reset, so write the
     * enable registers with absolute values rather than read-modify-write. */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_INT_EN0, IMU_SH3001_INT_TAP_ENABLE | IMU_SH3001_INT_ACTIVITY_ENABLE);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_INT_EN1, IMU_SH3001_INT_FREEFALL_ENABLE);
}

uint8_t imu_sh3001_motion_int_status(void)
{
    uint8_t status0 = imu_sh3001_read_byte(IMU_SH3001_REG_INT_STATUS0);

    (void)imu_sh3001_read_byte(IMU_SH3001_REG_INT_STATUS1);
    (void)imu_sh3001_read_byte(IMU_SH3001_REG_INT_STATUS2);
    (void)imu_sh3001_read_byte(IMU_SH3001_REG_TAP_STATUS);

    return status0;
}

void imu_sh3001_fifo_init(void)
{
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_FIFO_CFG_MODE, 0x80U);                 /* reset FIFO */
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_FIFO_CFG_WM_L, (uint8_t)(IMU_SH3001_FIFO_WATERMARK & 0xFFU));
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_FIFO_CFG_WM_H, (uint8_t)((IMU_SH3001_FIFO_WATERMARK >> 8) & 0x07U));
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_FIFO_CFG_DATA, IMU_SH3001_FIFO_DATA_ACC_GYRO);
    (void)imu_sh3001_write_byte(IMU_SH3001_REG_FIFO_CFG_MODE, 0x02U);                 /* stream mode */
}

uint16_t imu_sh3001_fifo_level(void)
{
    uint8_t lo = imu_sh3001_read_byte(IMU_SH3001_REG_FIFO_STATUS0);
    uint8_t hi = imu_sh3001_read_byte(IMU_SH3001_REG_FIFO_STATUS1);

    return (uint16_t)(((uint16_t)(hi & 0x07U) << 8) | lo);
}

uint8_t imu_sh3001_fifo_read(uint8_t *buf, uint16_t len)
{
    return imu_sh3001_read_nbytes(IMU_SH3001_REG_FIFO_DATA, buf, (uint8_t)len);
}
