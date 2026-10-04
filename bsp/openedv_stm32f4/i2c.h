/**
 * @file    i2c.h
 * @brief   IIC master on the ALIENTEK F429 IIC pins: SCL = PH4, SDA = PH5.
 *          Two backends are selectable with BSP_I2C_USE_HARDWARE: a software
 *          bit-bang master (default) or the hardware I2C2 peripheral (PH4/PH5,
 *          AF4). Both expose the same primitive API.
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/* 0 = software bit-bang (default), 1 = hardware I2C2 (PH4/PH5, AF4). */
#define BSP_I2C_USE_HARDWARE    0

/* Hardware backend bus speed (I2C2). AT24C02 supports up to 400 kHz. */
#define BSP_I2C_SPEED_HZ        400000U

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
