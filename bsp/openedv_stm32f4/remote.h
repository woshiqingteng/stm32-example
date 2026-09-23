/**
 * @file    remote.h
 * @brief   NEC infrared remote receiver on TIM1_CH1 (PA8) input capture.
 */

#ifndef BSP_REMOTE_H
#define BSP_REMOTE_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define REMOTE_IN_GPIO_PORT GPIOA
#define REMOTE_IN_GPIO_PIN  GPIO_PIN_8
#define REMOTE_IN_GPIO_AF   GPIO_AF1_TIM1
#define REMOTE_IN_TIMX      TIM1
#define REMOTE_IN_TIMX_CHY  TIM_CHANNEL_1

/** @brief  Remote control identification byte expected in the frame. */
#define REMOTE_ID           0U

/** @brief  NEC IR key codes for the ALIENTEK remote.
 *          0 (REMOTE_KEY_NONE) means "no key". */
typedef enum
{
    REMOTE_KEY_NONE     = 0U,
    REMOTE_KEY_VOL_DOWN = 7U,
    REMOTE_KEY_VOL_UP   = 9U,
    REMOTE_KEY_7        = 8U,
    REMOTE_KEY_4        = 12U,
    REMOTE_KEY_3        = 13U,
    REMOTE_KEY_UP       = 70U,
    REMOTE_KEY_DOWN     = 21U,
    REMOTE_KEY_1        = 22U,
    REMOTE_KEY_5        = 24U,
    REMOTE_KEY_2        = 25U,
    REMOTE_KEY_8        = 28U,
    REMOTE_KEY_0        = 66U,
    REMOTE_KEY_9        = 90U,
    REMOTE_KEY_6        = 94U,
    REMOTE_KEY_POWER    = 69U,
    REMOTE_KEY_PLAY     = 64U,
    REMOTE_KEY_ALIENTEK = 71U,
    REMOTE_KEY_RIGHT    = 67U,
    REMOTE_KEY_LEFT     = 68U,
    REMOTE_KEY_DELETE   = 74U
} remote_key_t;

/** @brief  Configure TIM1_CH1 input capture (1 tick = 1 us). */
void remote_init(void);

/** @brief  Return the key code of a newly decoded frame, or 0. */
uint8_t remote_scan(void);

/** @brief  Decode a raw 32-bit NEC frame into a key code (0 when invalid). */
uint8_t remote_parse(uint32_t frame);

/** @brief  Repeat count of the last key. */
uint8_t remote_repeat_count(void);

#endif /* BSP_REMOTE_H */
