/**
 * @file    qmi8658a.h
 * @brief   QMI8658A six-axis IMU driver (raw accelerometer / gyroscope counts
 *          and temperature). No attitude fusion is performed.
 */

#ifndef BSP_QMI8658A_H
#define BSP_QMI8658A_H

#include <stdint.h>

/** @brief  7-bit I2C address (SA0 = 1). */
#define QMI8658A_ADDR           0x6AU

#define QMI8658A_WHO_AM_I_VAL   0x05U
#define QMI8658A_REVISION_VAL   0x7CU

/* Register map. */
#define QMI8658A_REG_WHO_AM_I   0x00U
#define QMI8658A_REG_REVISION   0x01U
#define QMI8658A_REG_CTRL1      0x02U
#define QMI8658A_REG_CTRL2      0x03U
#define QMI8658A_REG_CTRL3      0x04U
#define QMI8658A_REG_CTRL5      0x06U
#define QMI8658A_REG_CTRL7      0x08U
#define QMI8658A_REG_STATUS0    0x2EU
#define QMI8658A_REG_TEMP_L     0x33U
#define QMI8658A_REG_AX_L       0x35U
#define QMI8658A_REG_RESET      0x60U

#define QMI8658A_RESET_CMD      0xB0U
#define QMI8658A_CTRL1_VALUE    0x60U
#define QMI8658A_CTRL5_VALUE    0x77U
#define QMI8658A_ENABLE_BOTH    0x03U
#define QMI8658A_STATUS_MASK    0x03U

/** @brief  Reset, probe and enable the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the device id does not match. */
uint8_t qmi8658a_init(void);

/** @brief  Read raw accelerometer and gyroscope counts. */
void qmi8658a_read_xyz(int16_t acc[3], int16_t gyro[3]);

/** @brief  Read the die temperature in degrees Celsius. */
float qmi8658a_read_temperature(void);

#endif /* BSP_QMI8658A_H */
