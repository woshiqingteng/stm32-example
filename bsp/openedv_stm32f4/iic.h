/**
 * @file    iic.h
 * @brief   Software (bit-bang) IIC master.
 *
 * Uses the ALIENTEK F429 board IIC pins: SCL = PH4, SDA = PH5. SDA is driven
 * open-drain so the bus is released high and the line can be read back.
 */

#ifndef BSP_IIC_H
#define BSP_IIC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define IIC_SCL_GPIO_PORT   GPIOH
#define IIC_SCL_GPIO_PIN    GPIO_PIN_4
#define IIC_SDA_GPIO_PORT   GPIOH
#define IIC_SDA_GPIO_PIN    GPIO_PIN_5

/** @brief  Configure SCL/SDA and release the bus (generate a STOP). */
void iic_init(void);

void iic_start(void);
void iic_stop(void);

/** @brief  Shift out one byte, MSB first. */
void iic_send_byte(uint8_t data);

/**
 * @brief  Clock in the slave ACK bit.
 * @return 0 if the slave acknowledged, 1 on timeout.
 */
uint8_t iic_wait_ack(void);

/**
 * @brief  Shift in one byte.
 * @param  ack 1: drive ACK after the byte; 0: drive NACK.
 */
uint8_t iic_read_byte(uint8_t ack);

void iic_ack(void);
void iic_nack(void);

#endif /* BSP_IIC_H */
