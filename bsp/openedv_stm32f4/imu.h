/**
 * @file    imu.h
 * @brief   Six-axis IMU: bias-corrected counts, temperature, motion interrupts
 *          and FIFO. Device-independent API over the chip driver (imu_sh3001.h).
 */

#ifndef BSP_IMU_H
#define BSP_IMU_H

#include <stdint.h>

/* Motion interrupt status bits (returned by imu_motion_int_status()). */
#define IMU_STATUS_TAP           0x10U   /*!< [4] tap (single or double) */
#define IMU_STATUS_ACTIVITY      0x02U   /*!< [1] */
#define IMU_STATUS_FREEFALL      0x80U   /*!< [7] */

/* FIFO frame: 6 axes x 2 bytes. */
#define IMU_FIFO_SAMPLE_LEN      12U

#define IMU_ACC_1G_COUNT         4096.0f /*!< counts per g at the configured +/-8g */
#define IMU_CAL_SAMPLE_COUNT     200U    /*!< samples averaged while calibrating */
#define IMU_CAL_SAMPLE_DELAY_MS  2U      /*!< settle delay between calibration samples */

#define IMU_DYN_ACC_MIN_G        0.9f    /*!< still detection: |acc| lower bound (g) */
#define IMU_DYN_ACC_MAX_G        1.1f    /*!< still detection: |acc| upper bound (g) */
#define IMU_DYN_GYRO_THR_COUNT   200     /*!< still detection: |gyro| limit (~3 dps) */
#define IMU_DYN_BIAS_ALPHA       0.01f   /*!< bias tracking gain when still */

/** @brief  Probe and configure the accelerometer and gyroscope.
 *  @return 0 on success, 1 if the chip id does not match. */
uint8_t imu_init(void);

/** @brief  Zero-bias calibration; keep the board still and level while it runs.
 *          Gyro bias is the averaged rate; acc bias keeps |acc| = 1g along the
 *          measured gravity direction. */
void imu_calibrate(void);

/** @brief  Slowly track the gyro bias while the board is stationary (call every
 *          sample with the bias-corrected values from imu_read_xyz()). */
void imu_update_dynamic_bias(const int16_t acc[3], const int16_t gyro[3]);

/** @brief  Read raw accelerometer and gyroscope counts (bias corrected). */
void imu_read_xyz(int16_t acc[3], int16_t gyro[3]);

/** @brief  Configure and enable the tap, free-fall and activity interrupts
 *          (routed to the INT pin). */
void imu_motion_int_enable(void);

/** @brief  Read and clear the motion interrupt status. */
uint8_t imu_motion_int_status(void);

/** @brief  Configure the FIFO (stream mode, acc + gyro) and enable it. */
void imu_fifo_init(void);

/** @brief  Number of samples currently in the FIFO. */
uint16_t imu_fifo_level(void);

/** @brief  Read @p len raw bytes from the FIFO data port. */
uint8_t imu_fifo_read(uint8_t *buf, uint16_t len);

/** @brief  Read the die temperature in degrees Celsius. */
float imu_read_temperature(void);

#endif /* BSP_IMU_H */
