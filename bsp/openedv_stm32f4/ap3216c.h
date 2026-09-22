/**
 * @file    ap3216c.h
 * @brief   AP3216C ambient light / proximity sensor driver.
 */

#ifndef BSP_AP3216C_H
#define BSP_AP3216C_H

#include <stdint.h>

/** @brief  8-bit I2C address (R/W bit added on the wire). */
#define AP3216C_ADDR    0x3CU

/* Register map. */
#define AP3216C_SYS_REG     0x00U
#define AP3216C_DATA_REG    0x0AU

#define AP3216C_RESET       0x04U
#define AP3216C_ALS_PS_IR   0x03U

/** @brief  Reset and configure the sensor.
 *  @return 0 on success, 1 if the system register did not read back. */
uint8_t ap3216c_init(void);

uint8_t ap3216c_write_one_byte(uint8_t reg, uint8_t data);
uint8_t ap3216c_read_one_byte(uint8_t reg);

/** @brief  Read the raw IR, PS and ALS channels. */
void ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als);

#endif /* BSP_AP3216C_H */
