/**
 * @file    imu.h
 * @brief   SH3001 six-axis IMU driver (raw accelerometer / gyroscope counts
 *          and temperature) on the board IIC bus (PH4 = SCL, PH5 = SDA).
 */

#ifndef BSP_IMU_H
#define BSP_IMU_H

#include <stdint.h>

/** @brief  7-bit I2C slave address (SDO pin tied to GND). */
#define IMU_ADDR                 0x36U

/** @brief  Expected value of the CHIP ID register. */
#define IMU_CHIP_ID_VAL          0x61U

/* Register map. */
#define IMU_REG_ACC_X_L          0x00U
#define IMU_REG_TEMP_L           0x0CU
#define IMU_REG_CHIP_ID          0x0FU
#define IMU_REG_TEMP_CONFIG0     0x20U
#define IMU_REG_TEMP_CONFIG1     0x21U
#define IMU_REG_ACC_CONFIG0      0x22U
#define IMU_REG_ACC_CONFIG1      0x23U
#define IMU_REG_ACC_CONFIG2      0x25U
#define IMU_REG_GYRO_CONFIG1     0x29U
#define IMU_REG_GYRO_CONFIG3_X   0x8FU
#define IMU_REG_GYRO_CONFIG3_Y   0x9FU
#define IMU_REG_GYRO_CONFIG3_Z   0xAFU
#define IMU_REG_TEMP_CONFIG2     0xD5U

/* Register field values. */
#define IMU_TEMP_ENABLE          0x80U   /*!< TEMP_CONFIG0[7]: digital sensor enable */
#define IMU_TEMP_ANALOG_MASK     0x04U   /*!< TEMP_CONFIG2[2]: 0 = analog sensor enable */
#define IMU_ACC_ODR_500HZ        0x01U   /*!< ACC_CONFIG1[3:0] */
#define IMU_ACC_RANGE_8G         0x03U   /*!< ACC_CONFIG2[2:0] */
#define IMU_GYRO_ODR_500HZ       0x01U   /*!< GYRO_CONFIG1[3:0] */
#define IMU_GYRO_RANGE_500DPS    0x04U   /*!< GYRO_CONFIG3[2:0] */

#define IMU_DATA_LEN_BYTE        12U     /*!< ACC (6) + GYRO (6) burst size */
#define IMU_TEMP_LEN_BYTE        2U
#define IMU_PROBE_RETRY_COUNT    5U

#define IMU_ACC_1G_COUNT         4096.0f /*!< counts per g at the configured +/-8g */
#define IMU_CAL_SAMPLE_COUNT     200U    /*!< samples averaged while calibrating */
#define IMU_CAL_SAMPLE_DELAY_MS  2U      /*!< settle delay between calibration samples */

/** @brief  Probe and configure the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the chip id does not match. */
uint8_t imu_init(void);

/** @brief  Zero-bias calibration; keep the board still and level while it runs.
 *          Gyro bias is the averaged rate; acc bias keeps |acc| = 1g along the
 *          measured gravity direction. */
void imu_calibrate(void);

/** @brief  Read raw accelerometer and gyroscope counts (bias corrected). */
void imu_read_xyz(int16_t acc[3], int16_t gyro[3]);

/** @brief  Read the die temperature in degrees Celsius. */
float imu_read_temperature(void);

#endif /* BSP_IMU_H */
