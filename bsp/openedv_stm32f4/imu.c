/**
 * @file    imu.c
 * @brief   SH3001 six-axis IMU driver.
 *
 * Only raw sensor access is provided: accelerometer and gyroscope counts plus
 * the die temperature. The accelerometer is configured for +/-8g and the
 * gyroscope for +/-500dps, both at 500Hz.
 */

#include <math.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "imu.h"
#include "delay.h"

#define IMU_TEMP_DIVISOR   16.0f  /*!< TEMP DATA resolution, LSB per degC */
#define IMU_TEMP_OFFSET_C  25.0f  /*!< reference temperature, degC */

static int16_t g_imu_acc[3];
static int16_t g_imu_gyro[3];
static float   g_imu_acc_bias[3];
static float   g_imu_gyro_bias[3];

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

void imu_calibrate(void)
{
    int32_t  sum_acc[3] = { 0, 0, 0 };
    int32_t  sum_gyro[3] = { 0, 0, 0 };
    int16_t  acc[3];
    int16_t  gyro[3];
    float    ax, ay, az;
    float    mag;
    float    scale;
    uint16_t i;

    for (i = 0U; i < IMU_CAL_SAMPLE_COUNT; i++)
    {
        imu_read_xyz(acc, gyro);

        sum_acc[0] += acc[0];
        sum_acc[1] += acc[1];
        sum_acc[2] += acc[2];
        sum_gyro[0] += gyro[0];
        sum_gyro[1] += gyro[1];
        sum_gyro[2] += gyro[2];

        delay_ms(IMU_CAL_SAMPLE_DELAY_MS);
    }

    g_imu_gyro_bias[0] = (float)sum_gyro[0] / (float)IMU_CAL_SAMPLE_COUNT;
    g_imu_gyro_bias[1] = (float)sum_gyro[1] / (float)IMU_CAL_SAMPLE_COUNT;
    g_imu_gyro_bias[2] = (float)sum_gyro[2] / (float)IMU_CAL_SAMPLE_COUNT;

    /* Keep the measured gravity vector at 1g (orientation independent). */
    ax = (float)sum_acc[0] / (float)IMU_CAL_SAMPLE_COUNT;
    ay = (float)sum_acc[1] / (float)IMU_CAL_SAMPLE_COUNT;
    az = (float)sum_acc[2] / (float)IMU_CAL_SAMPLE_COUNT;
    mag = sqrtf(ax * ax + ay * ay + az * az);

    if (mag > 1.0f)
    {
        scale = IMU_ACC_1G_COUNT / mag;
        g_imu_acc_bias[0] = ax * (1.0f - scale);
        g_imu_acc_bias[1] = ay * (1.0f - scale);
        g_imu_acc_bias[2] = az * (1.0f - scale);
    }
    else
    {
        g_imu_acc_bias[0] = 0.0f;
        g_imu_acc_bias[1] = 0.0f;
        g_imu_acc_bias[2] = 0.0f;
    }
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

    acc[0] = (int16_t)((float)g_imu_acc[0] - g_imu_acc_bias[0]);
    acc[1] = (int16_t)((float)g_imu_acc[1] - g_imu_acc_bias[1]);
    acc[2] = (int16_t)((float)g_imu_acc[2] - g_imu_acc_bias[2]);

    gyro[0] = (int16_t)((float)g_imu_gyro[0] - g_imu_gyro_bias[0]);
    gyro[1] = (int16_t)((float)g_imu_gyro[1] - g_imu_gyro_bias[1]);
    gyro[2] = (int16_t)((float)g_imu_gyro[2] - g_imu_gyro_bias[2]);
}

void imu_update_dynamic_bias(const int16_t acc[3], const int16_t gyro[3])
{
    float   ax = (float)acc[0] / IMU_ACC_1G_COUNT;
    float   ay = (float)acc[1] / IMU_ACC_1G_COUNT;
    float   az = (float)acc[2] / IMU_ACC_1G_COUNT;
    float   mag = sqrtf(ax * ax + ay * ay + az * az);
    uint8_t i;

    if ((mag < IMU_DYN_ACC_MIN_G) || (mag > IMU_DYN_ACC_MAX_G))
    {
        return;
    }

    for (i = 0U; i < 3U; i++)
    {
        if (((gyro[i] < 0) ? -gyro[i] : gyro[i]) >= IMU_DYN_GYRO_THR_COUNT)
        {
            return;
        }
    }

    for (i = 0U; i < 3U; i++)
    {
        g_imu_gyro_bias[i] += IMU_DYN_BIAS_ALPHA * (float)gyro[i];
    }
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

void imu_motion_int_enable(void)
{
    /* Tap: X/Y/Z, |acc|/64 above 0x51 within 0x52 ms. */
    (void)imu_write_byte(IMU_REG_TAP_AXIS, 0x0EU);
    (void)imu_write_byte(IMU_REG_TAP_THR, 0x10U);
    (void)imu_write_byte(IMU_REG_TAP_DUR, 0x50U);
    (void)imu_write_byte(IMU_REG_TAP_LAT, 0x40U);
    (void)imu_write_byte(IMU_REG_TAP_WIN, 0x80U);

    /* Free-fall: ~300mg threshold at 2mg/LSB, 160ms (2ms/LSB). */
    (void)imu_write_byte(IMU_REG_FREEFALL_THR, 0x96U);
    (void)imu_write_byte(IMU_REG_FREEFALL_TIME, 0x50U);

    /* Activity: X/Y/Z in AC mode (delta of consecutive samples, 0.97mg/LSB at
     * +/-8g), ~31mg threshold over 3 samples. */
    (void)imu_write_byte(IMU_REG_ACT_AXIS, 0xF0U);
    (void)imu_write_byte(IMU_REG_ACT_THR, 0x20U);
    (void)imu_write_byte(IMU_REG_ACT_TIME, 0x03U);

    /* Interrupt config: latched, active high, normal output (cleared by
     * reading the interrupt status register). */
    (void)imu_write_byte(IMU_REG_INT_CONFIG, 0x05U);

    /* The SH3001 keeps its configuration across an MCU reset, so write the
     * enable registers with absolute values rather than read-modify-write. */
    (void)imu_write_byte(IMU_REG_INT_EN0, IMU_INT_TAP_ENABLE | IMU_INT_ACTIVITY_ENABLE);
    (void)imu_write_byte(IMU_REG_INT_EN1, IMU_INT_FREEFALL_ENABLE);
}

uint8_t imu_motion_int_status(void)
{
    uint8_t status0 = imu_read_byte(IMU_REG_INT_STATUS0);

    (void)imu_read_byte(IMU_REG_INT_STATUS1);
    (void)imu_read_byte(IMU_REG_INT_STATUS2);
    (void)imu_read_byte(IMU_REG_TAP_STATUS);

    return status0;
}

void imu_fifo_init(void)
{
    (void)imu_write_byte(IMU_REG_FIFO_CFG_MODE, 0x80U);                 /* reset FIFO */
    (void)imu_write_byte(IMU_REG_FIFO_CFG_WM_L, (uint8_t)(IMU_FIFO_WATERMARK & 0xFFU));
    (void)imu_write_byte(IMU_REG_FIFO_CFG_WM_H, (uint8_t)((IMU_FIFO_WATERMARK >> 8) & 0x07U));
    (void)imu_write_byte(IMU_REG_FIFO_CFG_DATA, IMU_FIFO_DATA_ACC_GYRO);
    (void)imu_write_byte(IMU_REG_FIFO_CFG_MODE, 0x02U);                 /* stream mode */
}

uint16_t imu_fifo_level(void)
{
    uint8_t lo = imu_read_byte(IMU_REG_FIFO_STATUS0);
    uint8_t hi = imu_read_byte(IMU_REG_FIFO_STATUS1);

    return (uint16_t)(((uint16_t)(hi & 0x07U) << 8) | lo);
}

uint8_t imu_fifo_read(uint8_t *buf, uint16_t len)
{
    return imu_read_nbytes(IMU_REG_FIFO_DATA, buf, (uint8_t)len);
}
