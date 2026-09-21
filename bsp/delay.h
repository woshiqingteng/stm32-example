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

/** @brief  Blocking microsecond delay. @param nus Delay in microseconds. */
void delay_us(uint32_t nus);

/** @brief  Blocking millisecond delay. @param nms Delay in milliseconds. */
void delay_ms(uint16_t nms);

#endif /* BSP_DELAY_H */
