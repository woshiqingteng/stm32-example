/**
 * @file    key.h
 * @brief   On-board keys interface.
 */

#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdbool.h>
#include "stm32f4xx_hal.h"

typedef enum
{
    KEY0 = 0,
    KEY1 = 1,
    KEY2 = 2,
    KEY_WKUP = 3,
    KEY_NUM = 4,        /*!< number of physical keys */
    KEY_NONE = 4        /*!< scan result: nothing pressed */
} key_id_t;

/** @brief  Initialise the key GPIOs. */
void key_init(void);

/** @brief  GPIO port of a key (for EXTI wiring). */
GPIO_TypeDef *key_port(key_id_t id);

/** @brief  GPIO pin of a key (for EXTI wiring). */
uint16_t key_pin(key_id_t id);

/** @brief  True when the key is currently pressed (for debounce re-check). */
bool key_is_pressed(key_id_t id);

/**
 * @brief  Scan keys with 10 ms debounce.
 * @param  continuous true: report every press; false: latch until release.
 * @return Pressed key id, or KEY_NONE.
 */
key_id_t key_scan(bool continuous);

#endif /* BSP_KEY_H */
