/**
 * @file    ano.h
 * @brief   ANO_TC v4 telemetry upload over USART1.
 *
 * Frame: [0xAA][0xAA][fun][len][data...][checksum], checksum = sum of the
 * first (len + 4) bytes.
 */

#ifndef ANO_H
#define ANO_H

#include <stdint.h>

/** @brief  Upload the attitude (fun 0x01): roll, pitch, yaw in degrees. */
void ano_report_imu(float roll, float pitch, float yaw);

/** @brief  Upload the raw sensor counts (fun 0x02). */
void ano_report_raw(const int16_t acc[3], const int16_t gyro[3]);

/** @brief  Upload the orientation quaternion (fun 0x04): w, x, y, z. */
void ano_report_quat(float w, float x, float y, float z);

#endif /* ANO_H */
