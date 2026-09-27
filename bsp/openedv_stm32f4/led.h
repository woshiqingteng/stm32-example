/**
 * @file    led.h
 * @brief   On-board LED interface (active low).
 */

#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdbool.h>

/** @brief On-board LEDs: LED0 = PB1 (RED), LED1 = PB0 (GREEN). */
typedef enum
{
    LED0 = 0,
    LED1 = 1,
} led_id_t;

/** @brief  Initialise the LED GPIOs (both off). */
void led_init(void);

void led_on(led_id_t id);
void led_off(led_id_t id);
void led_toggle(led_id_t id);

/** @brief  Return true when the LED is lit (active low). */
bool led_is_on(led_id_t id);

/** @brief  Release an LED pin into a floating input (e.g. to observe a jumper). */
void led_set_input(led_id_t id);

#endif /* BSP_LED_H */
