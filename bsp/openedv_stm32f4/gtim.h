/**
 * @file    gtim.h
 * @brief   General timer interface: TIM3/TIM5/TIM2 int / pwm / capture / counter,
 *          plus the TIM14 1 Hz frame-rate counter.
 */

#ifndef BSP_GTIM_H
#define BSP_GTIM_H

#include <stdint.h>

/** @brief Callback invoked from the TIM3 update interrupt. */
typedef void (*gtim_cb_t)(void);

/* ---- TIM3 update interrupt ---- */
void gtim_timx_int_init(uint16_t arr, uint16_t psc);
void gtim_timx_int_register(gtim_cb_t cb);

/* ---- TIM3_CH4 (PB1) PWM ---- */
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);
void gtim_timx_pwm_chy_set(uint16_t ccr);

/* ---- TIM5_CH1 (PA0) input capture (32-bit timer + software overflow) ---- */
typedef enum
{
    GTIM_CAP_RISING   = 0,   /*!< rising edge  (arm + event) */
    GTIM_CAP_FALLING  = 1,   /*!< falling edge (arm + event) */
    GTIM_CAP_OVERFLOW = 2,   /*!< 32-bit CNT wrapped (event only) */
} gtim_cap_event_t;

/** @brief  Called on every event; returns the edge to arm next
 *          (ignored when the event is GTIM_CAP_OVERFLOW). */
typedef gtim_cap_event_t (*gtim_cap_cb_t)(uint32_t value, gtim_cap_event_t event);

void gtim_timx_cap_chy_init(uint32_t arr, uint16_t psc);
void gtim_timx_cap_chy_register(gtim_cap_cb_t cb);

/* ---- TIM2_CH1 (PA0) external pulse counter ---- */
void gtim_timx_cnt_chy_init(uint16_t psc);
uint64_t gtim_timx_cnt_chy_get_count(void);
void gtim_timx_cnt_chy_restart(void);

/* ---- TIM14 1 Hz frame-rate timer (camera) ---- */
/** @brief  Start TIM14 with a 1 Hz update interrupt. */
void     gtim_frame_init(void);

/** @brief  Count one captured frame (called from the DCMI frame hook). */
void     gtim_frame_inc(void);

/** @brief  Total frames counted since gtim_frame_init(). */
uint32_t gtim_frame_count(void);

/** @brief  Frames captured during the last completed one-second window. */
uint32_t gtim_frame_rate(void);

/** @brief  Whole seconds elapsed since gtim_frame_init(). */
uint32_t gtim_frame_uptime(void);

#endif /* BSP_GTIM_H */
