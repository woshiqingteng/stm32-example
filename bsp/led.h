/**
 * @file    led.h
 * @brief   On-board LED interface (active low).
 */

#ifndef BSP_LED_H
#define BSP_LED_H

/** @brief On-board LEDs: LED0 = PB1 (RED), LED1 = PB0 (GREEN). */
typedef enum
{
    LED0 = 0,
    LED1,
} led_id_t;

/** @brief  Initialise the LED GPIOs (both off). */
void led_init(void);

/** @brief  Turn an LED on. @param id LED index. */
void led_on(led_id_t id);

/** @brief  Turn an LED off. @param id LED index. */
void led_off(led_id_t id);

/** @brief  Toggle an LED. @param id LED index. */
void led_toggle(led_id_t id);

#endif /* BSP_LED_H */
