/**
 * @file    pcf8574.h
 * @brief   PCF8574 8-bit I2C IO expander driver.
 *
 * The device sits on the shared software IIC bus (PH4/PH5). Its INT output is
 * wired to PB12 and is active low.
 */

#ifndef BSP_PCF8574_H
#define BSP_PCF8574_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define PCF8574_GPIO_PORT   GPIOB
#define PCF8574_GPIO_PIN    GPIO_PIN_12

#define PCF8574_INT         HAL_GPIO_ReadPin(PCF8574_GPIO_PORT, PCF8574_GPIO_PIN)

/** @brief  8-bit write address (the R/W bit is added on the wire). */
#define PCF8574_ADDR        0x40U

/* Expansion port bit assignment on the ALIENTEK F429 board. */
#define PCF8574_BEEP_IO     0U
#define PCF8574_AP_INT_IO   1U
#define PCF8574_DCMI_PWDN_IO 2U
#define PCF8574_USB_PWR_IO  3U
#define PCF8574_EX_IO       4U
#define PCF8574_MPU_INT_IO  5U
#define PCF8574_RS485_RE_IO 6U
#define PCF8574_ETH_RESET_IO 7U

/** @brief  Initialise the INT input and the IIC bus, then release all outputs.
 *  @return 0 if the expander acknowledged, 1 otherwise. */
uint8_t pcf8574_init(void);

uint8_t pcf8574_read_byte(void);
void    pcf8574_write_byte(uint8_t data);

/** @brief  True when the active-low INT output is asserted. */
bool    pcf8574_int_asserted(void);

void    pcf8574_write_bit(uint8_t bit, uint8_t sta);
uint8_t pcf8574_read_bit(uint8_t bit);

#endif /* BSP_PCF8574_H */
