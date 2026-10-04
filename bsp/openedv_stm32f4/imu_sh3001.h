/**
 * @file    imu_sh3001.h
 * @brief   SH3001 six-axis IMU chip driver (raw accelerometer / gyroscope
 *          counts and temperature) on the board IIC bus (PH4 = SCL, PH5 = SDA).
 */

#ifndef BSP_IMU_SH3001_H
#define BSP_IMU_SH3001_H

#include <stdint.h>

/** @brief  Expected value of the CHIP ID register. */
#define IMU_SH3001_CHIP_ID_VAL          0x61U

/* Register map. */
#define IMU_SH3001_REG_ACC_X_L          0x00U
#define IMU_SH3001_REG_TEMP_L           0x0CU
#define IMU_SH3001_REG_CHIP_ID          0x0FU
#define IMU_SH3001_REG_TEMP_CONFIG0     0x20U
#define IMU_SH3001_REG_TEMP_CONFIG1     0x21U
#define IMU_SH3001_REG_ACC_CONFIG0      0x22U
#define IMU_SH3001_REG_ACC_CONFIG1      0x23U
#define IMU_SH3001_REG_ACC_CONFIG2      0x25U
#define IMU_SH3001_REG_GYRO_CONFIG1     0x29U
#define IMU_SH3001_REG_GYRO_CONFIG3_X   0x8FU
#define IMU_SH3001_REG_GYRO_CONFIG3_Y   0x9FU
#define IMU_SH3001_REG_GYRO_CONFIG3_Z   0xAFU
#define IMU_SH3001_REG_TEMP_CONFIG2     0xD5U

/* Motion interrupt registers. */
#define IMU_SH3001_REG_INT_STATUS0      0x10U
#define IMU_SH3001_REG_INT_STATUS1      0x11U
#define IMU_SH3001_REG_INT_STATUS2      0x12U
#define IMU_SH3001_REG_TAP_STATUS       0x14U
#define IMU_SH3001_REG_INT_EN0          0x40U
#define IMU_SH3001_REG_INT_EN1          0x41U
#define IMU_SH3001_REG_INT_CONFIG       0x44U
#define IMU_SH3001_REG_ACT_AXIS         0x4FU
#define IMU_SH3001_REG_TAP_AXIS         0x50U
#define IMU_SH3001_REG_TAP_THR          0x51U
#define IMU_SH3001_REG_TAP_DUR          0x52U
#define IMU_SH3001_REG_TAP_LAT          0x53U
#define IMU_SH3001_REG_TAP_WIN          0x54U
#define IMU_SH3001_REG_ACT_THR          0x55U
#define IMU_SH3001_REG_ACT_TIME         0x56U
#define IMU_SH3001_REG_FREEFALL_THR     0x5EU
#define IMU_SH3001_REG_FREEFALL_TIME    0x5FU

/* FIFO registers. */
#define IMU_SH3001_REG_FIFO_STATUS0     0x16U
#define IMU_SH3001_REG_FIFO_STATUS1     0x17U
#define IMU_SH3001_REG_FIFO_DATA        0x18U
#define IMU_SH3001_REG_FIFO_CFG_MODE    0x35U
#define IMU_SH3001_REG_FIFO_CFG_WM_L    0x36U
#define IMU_SH3001_REG_FIFO_CFG_WM_H    0x37U
#define IMU_SH3001_REG_FIFO_CFG_DATA    0x38U

/* Register field values. */
#define IMU_SH3001_TEMP_ENABLE          0x80U   /*!< TEMP_CONFIG0[7]: digital sensor enable */
#define IMU_SH3001_TEMP_ANALOG_MASK     0x04U   /*!< TEMP_CONFIG2[2]: 0 = analog sensor enable */
#define IMU_SH3001_ACC_ODR_500HZ        0x01U   /*!< ACC_CONFIG1[3:0] */
#define IMU_SH3001_ACC_RANGE_8G         0x03U   /*!< ACC_CONFIG2[2:0] */
#define IMU_SH3001_GYRO_ODR_500HZ       0x01U   /*!< GYRO_CONFIG1[3:0] */
#define IMU_SH3001_GYRO_RANGE_500DPS    0x04U   /*!< GYRO_CONFIG3[2:0] */

/* Motion interrupt enable bits. */
#define IMU_SH3001_INT_TAP_ENABLE       0x04U   /*!< INT_EN0[2] */
#define IMU_SH3001_INT_ACTIVITY_ENABLE  0x10U   /*!< INT_EN0[4] */
#define IMU_SH3001_INT_FREEFALL_ENABLE  0x01U   /*!< INT_EN1[0] */

/* FIFO configuration. */
#define IMU_SH3001_FIFO_DATA_ACC_GYRO   0x3FU   /*!< acc X/Y/Z + gyro X/Y/Z into FIFO */
#define IMU_SH3001_FIFO_WATERMARK       16U

#define IMU_SH3001_DATA_LEN_BYTE        12U     /*!< ACC (6) + GYRO (6) burst size */
#define IMU_SH3001_TEMP_LEN_BYTE        2U
#define IMU_SH3001_PROBE_RETRY_COUNT    5U

/** @brief  Probe and configure the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the chip id does not match. */
uint8_t imu_sh3001_init(void);

/** @brief  Read raw accelerometer and gyroscope counts. @return 0 on success. */
uint8_t imu_sh3001_read_raw(int16_t acc[3], int16_t gyro[3]);

/** @brief  Read the die temperature in degrees Celsius. */
float imu_sh3001_read_temperature(void);

/** @brief  Configure and enable the tap, free-fall and activity interrupts
 *          (routed to the INT pin). */
void imu_sh3001_motion_int_enable(void);

/** @brief  Read and clear the motion interrupt status. */
uint8_t imu_sh3001_motion_int_status(void);

/** @brief  Configure the FIFO (stream mode, acc + gyro) and enable it. */
void imu_sh3001_fifo_init(void);

/** @brief  Number of samples currently in the FIFO. */
uint16_t imu_sh3001_fifo_level(void);

/** @brief  Read @p len raw bytes from the FIFO data port. */
uint8_t imu_sh3001_fifo_read(uint8_t *buf, uint16_t len);

#endif /* BSP_IMU_SH3001_H */
