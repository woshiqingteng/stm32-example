/**
 * @file    dht11.h
 * @brief   DHT11 digital temperature / humidity sensor (DQ = PB12).
 */

#ifndef BSP_DHT11_H
#define BSP_DHT11_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define DHT11_DQ_GPIO_PORT  GPIOB
#define DHT11_DQ_GPIO_PIN   GPIO_PIN_12

/** @brief  Reset the bus and probe the device. @return 0 if present. */
uint8_t dht11_init(void);

/** @brief  Wait for the DHT11 response. @return 0 if the device responded. */
uint8_t dht11_check(void);

/**
 * @brief  Read one measurement.
 * @param  temp temperature in degrees Celsius (0..50)
 * @param  humi relative humidity in percent (20..90)
 * @return 0 on success, 1 on checksum/response failure
 */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi);

#endif /* BSP_DHT11_H */
