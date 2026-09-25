/**
 * @file    ir.h
 * @brief   NEC infrared ir receiver on TIM1_CH1 (PA8) input capture.
 */

#ifndef BSP_IR_H
#define BSP_IR_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define IR_IN_GPIO_PORT GPIOA
#define IR_IN_GPIO_PIN  GPIO_PIN_8
#define IR_IN_GPIO_AF   GPIO_AF1_TIM1
#define IR_IN_TIMX      TIM1
#define IR_IN_TIMX_CHY  TIM_CHANNEL_1

/** @brief  Ir control identification byte expected in the frame. */
#define IR_ID           0U

/** @brief  NEC IR key codes for the ALIENTEK ir.
 *          0 (IR_KEY_NONE) means "no key". */
typedef enum
{
    IR_KEY_NONE     = 0U,
    IR_KEY_VOL_DOWN = 7U,
    IR_KEY_VOL_UP   = 9U,
    IR_KEY_7        = 8U,
    IR_KEY_4        = 12U,
    IR_KEY_3        = 13U,
    IR_KEY_UP       = 70U,
    IR_KEY_DOWN     = 21U,
    IR_KEY_1        = 22U,
    IR_KEY_5        = 24U,
    IR_KEY_2        = 25U,
    IR_KEY_8        = 28U,
    IR_KEY_0        = 66U,
    IR_KEY_9        = 90U,
    IR_KEY_6        = 94U,
    IR_KEY_POWER    = 69U,
    IR_KEY_PLAY     = 64U,
    IR_KEY_ALIENTEK = 71U,
    IR_KEY_RIGHT    = 67U,
    IR_KEY_LEFT     = 68U,
    IR_KEY_DELETE   = 74U
} ir_key_t;

/** @brief  Configure TIM1_CH1 input capture (1 tick = 1 us). */
void ir_init(void);

/** @brief  Return the key code of a newly decoded frame, or 0. */
uint8_t ir_scan(void);

/** @brief  Decode a raw 32-bit NEC frame into a key code (0 when invalid). */
uint8_t ir_parse(uint32_t frame);

/** @brief  Repeat count of the last key. */
uint8_t ir_repeat_count(void);

#endif /* BSP_IR_H */
