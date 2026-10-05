/**
 * @file    delay.h
 * @brief   SysTick polling delays.
 */

#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include <stdint.h>

#ifndef USE_FREERTOS
#define USE_FREERTOS 0
#endif

/** @brief  Initialise the delay multiplier. @param sysclk HCLK frequency in MHz. */
void delay_init(uint16_t sysclk);

/** @brief  Busy-wait delay (no scheduler API; safe in critical/ISR). @param nus Microseconds. */
void delay_us(uint32_t nus);

/** @brief  Busy-wait delay (no scheduler API; safe in critical/ISR). @param nms Milliseconds. */
void delay_ms(uint16_t nms);

/** @brief  Monotonic microsecond timestamp (32-bit; wraps ~71 min). */
uint32_t delay_us_now(void);

#endif /* BSP_DELAY_H */
