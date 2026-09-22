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

extern uint8_t g_remote_cnt; /*!< repeat count of the last key */

/** @brief  Configure TIM1_CH1 input capture (1 tick = 1 us). */
void remote_init(void);

/** @brief  Return the key code of a newly decoded frame, or 0. */
uint8_t remote_scan(void);

/** @brief  Decode a raw 32-bit NEC frame into a key code (0 when invalid). */
uint8_t remote_parse(uint32_t frame);

#endif /* BSP_REMOTE_H */
