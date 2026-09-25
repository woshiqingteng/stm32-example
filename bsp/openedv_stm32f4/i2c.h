/**
 * @file    i2c.h
 * @brief   Software (bit-bang) IIC master.
 *
 * Uses the ALIENTEK F429 board IIC pins: SCL = PH4, SDA = PH5. SDA is driven
 * open-drain so the bus is released high and the line can be read back.
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define I2C_SCL_GPIO_PORT   GPIOH
#define I2C_SCL_GPIO_PIN    GPIO_PIN_4
#define I2C_SDA_GPIO_PORT   GPIOH
#define I2C_SDA_GPIO_PIN    GPIO_PIN_5

/** @brief  Configure SCL/SDA and release the bus (generate a STOP). */
void i2c_init(void);

void i2c_start(void);
void i2c_stop(void);

/** @brief  Shift out one byte, MSB first. */
void i2c_send_byte(uint8_t data);

/**
 * @brief  Clock in the slave ACK bit.
 * @return 0 if the slave acknowledged, 1 on timeout.
 */
uint8_t i2c_wait_ack(void);

/**
 * @brief  Shift in one byte.
 * @param  ack 1: drive ACK after the byte; 0: drive NACK.
 */
uint8_t i2c_read_byte(uint8_t ack);

void i2c_ack(void);
void i2c_nack(void);

#endif /* BSP_I2C_H */
