/**
 * @file    timer.h
 * @brief   Camera frame-rate timer (TIM6, 1 Hz update interrupt).
 *
 * The DCMI frame hook calls timer_frame_inc(); every second the update ISR
 * latches the frame count into the frame-rate register and clears the running
 * counter, so timer_frame_rate() always reports the last complete second.
 */

#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#include <stdint.h>

/** @brief  Start TIM6 with a 1 Hz update interrupt. */
void timer_init(void);

/** @brief  Count one captured frame (called from the DCMI frame hook). */
void timer_frame_inc(void);

/** @brief  Total frames counted since timer_init(). */
uint32_t timer_frame_count(void);

/** @brief  Frames captured during the last completed one-second window. */
uint32_t timer_frame_rate(void);

/** @brief  Whole seconds elapsed since timer_init(). */
uint32_t timer_uptime(void);

#endif /* BSP_TIMER_H */
