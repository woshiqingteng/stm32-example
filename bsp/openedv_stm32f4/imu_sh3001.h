/**
 * @file    imu_sh3001.h
 * @brief   SH3001 six-axis IMU chip driver (raw accelerometer / gyroscope
 *          counts and temperature) on the board IIC bus (PH4 = SCL, PH5 = SDA).
 */

#ifndef BSP_IMU_SH3001_H
#define BSP_IMU_SH3001_H

#include <stdint.h>

/* SH3001 native (raw) axes on this board (board laid flat, face up):
 *   +X = across the board (from the IIC edge to the opposite edge),
 *   +Y = along the long edge, +Z = out of the top face (up).
 * An axis held pointing up reads +1 g on the accelerometer. */

/* Data layout (16-bit little-endian per axis, low byte first):
 *   0x00/01 X, 0x02/03 Y, 0x04/05 Z : accelerometer (+/-8g, 4096 LSB/g)
 *   0x06/07 X, 0x08/09 Y, 0x0A/0B Z : gyroscope (+/-500dps, 65.5 LSB/dps)
 *   0x0C low8 + 0x0D[3:0] high     : temperature (16 LSB/degC)
 *   0x0F chip id = 0x61. */
#define IMU_SH3001_MK16(lo, hi)         ((int16_t)(((uint16_t)(hi) << 8) | (uint8_t)(lo)))

/** @brief  Expected value of the CHIP ID register. */
#define IMU_SH3001_CHIP_ID_VAL          0x61U

/* Register map. */
#define IMU_SH3001_REG_ACC_X_L          0x00U
#define IMU_SH3001_REG_TEMP_L           0x0CU
#define IMU_SH3001_REG_TEMP_H           0x0DU
#define IMU_SH3001_REG_CHIP_ID          0x0FU
#define IMU_SH3001_REG_TEMP_CONFIG0     0x20U
#define IMU_SH3001_REG_TEMP_CONFIG1     0x21U
#define IMU_SH3001_REG_ACC_CONFIG0      0x22U
#define IMU_SH3001_REG_ACC_CONFIG1      0x23U
#define IMU_SH3001_REG_ACC_CONFIG2      0x25U
#define IMU_SH3001_REG_ACC_CONFIG3      0x26U
#define IMU_SH3001_REG_GYRO_CONFIG1     0x29U
#define IMU_SH3001_REG_GYRO_CONFIG2     0x2BU
#define IMU_SH3001_REG_GYRO_CONFIG3_X   0x8FU
#define IMU_SH3001_REG_GYRO_CONFIG3_Y   0x9FU
#define IMU_SH3001_REG_GYRO_CONFIG3_Z   0xAFU
#define IMU_SH3001_REG_TEMP_CONFIG2     0xD5U

/* Register field values. */
#define IMU_SH3001_TEMP_ENABLE          0x80U
#define IMU_SH3001_TEMP_ANALOG_MASK     0x04U
#define IMU_SH3001_ACC_ODR_125HZ        0x03U
#define IMU_SH3001_ACC_RANGE_8G         0x03U
#define IMU_SH3001_ACC_LPF_ODR_025      0x20U
#define IMU_SH3001_GYRO_ODR_125HZ       0x03U
#define IMU_SH3001_GYRO_RANGE_125DPS    0x02U
#define IMU_SH3001_GYRO_LPF_01          0x04U

#define IMU_SH3001_DATA_LEN_BYTE        12U
#define IMU_SH3001_TEMP_LEN_BYTE        2U
#define IMU_SH3001_PROBE_RETRY_COUNT    5U

/** @brief  Probe and configure the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the chip id does not match. */
uint8_t imu_sh3001_init(void);

/** @brief  Read raw accelerometer/gyroscope counts and the die temperature.
 *  @return 0 on success. */
uint8_t imu_sh3001_read(int16_t acc[3], int16_t gyro[3], float *temperature);

#endif /* BSP_IMU_SH3001_H */
