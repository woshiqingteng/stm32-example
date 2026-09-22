/**
 * @file    ds18b20.h
 * @brief   DS18B20 1-Wire digital temperature sensor (DQ = PB12).
 */

#ifndef BSP_DS18B20_H
#define BSP_DS18B20_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define DS18B20_DQ_GPIO_PORT    GPIOB
#define DS18B20_DQ_GPIO_PIN     GPIO_PIN_12

/** @brief  Reset the bus and probe the device. @return 0 if present. */
uint8_t ds18b20_init(void);

/** @brief  Wait for a presence pulse. @return 0 if the device responded. */
uint8_t ds18b20_check(void);

/** @brief  Start a conversion and return the temperature in tenths of a degree. */
int16_t ds18b20_get_temperature(void);

#endif /* BSP_DS18B20_H */
