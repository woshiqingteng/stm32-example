/**
 * @file    pwmdac.h
 * @brief   PWM DAC interface: TIM9_CH2 (PA3) filtered PWM used as a DAC.
 */

#ifndef BSP_PWMDAC_H
#define BSP_PWMDAC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief PWM DAC reference voltage in mV (full-scale duty). */
#define PWMDAC_VREF_MV 3300U

/** @brief  Initialise TIM9_CH2 (PA3) PWM at the given period/prescaler. */
void pwmdac_init(uint16_t arr, uint16_t psc);

/** @brief  Set the output voltage: vol is 0..3300 mV, mapped to the duty cycle. */
void pwmdac_set(uint16_t vol);

/** @brief  Read back the current timer compare (CCR) value. */
uint16_t pwmdac_get_code(void);

#endif /* BSP_PWMDAC_H */
