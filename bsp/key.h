/**
 * @file    key.h
 * @brief   On-board keys interface.
 */

#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* On-board keys: KEY0 = PH3, KEY1 = PH2, KEY2 = PC13, WK_UP = PA0. */
#define KEY0_GPIO_PORT      GPIOH
#define KEY0_GPIO_PIN       GPIO_PIN_3
#define KEY1_GPIO_PORT      GPIOH
#define KEY1_GPIO_PIN       GPIO_PIN_2
#define KEY2_GPIO_PORT      GPIOC
#define KEY2_GPIO_PIN       GPIO_PIN_13
#define KEY_WKUP_GPIO_PORT  GPIOA
#define KEY_WKUP_GPIO_PIN   GPIO_PIN_0

typedef enum
{
    KEY0 = 0,
    KEY1,
    KEY2,
    KEY_WKUP,
    KEY_NUM,           /*!< number of physical keys */
    KEY_NONE = KEY_NUM /*!< scan result: nothing pressed */
} key_id_t;

typedef enum
{
    KEY_RELEASED = 0,
    KEY_PRESSED,
} key_state_t;

/** @brief  Initialise the key GPIOs. */
void key_init(void);

/** @brief  Read the current state of a key (KEY_PRESSED / KEY_RELEASED). */
key_state_t key_read(key_id_t id);

/**
 * @brief  Scan keys with 10 ms debounce.
 * @param  continuous true: report every press; false: latch until release.
 * @return Pressed key id, or KEY_NONE.
 */
key_id_t key_scan(bool continuous);

#endif /* BSP_KEY_H */
