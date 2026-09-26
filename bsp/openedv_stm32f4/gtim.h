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

typedef enum
{
    GTIM_CAP_IDLE = 0, /*!< waiting for the rising edge */
    GTIM_CAP_RISING,   /*!< rising edge captured, waiting for falling */
    GTIM_CAP_DONE,     /*!< high-level width available */
} gtim_cap_state_t;

/* ---- TIM3 update interrupt ---- */
void gtim_timx_int_init(uint16_t arr, uint16_t psc);
void gtim_timx_int_register(gtim_cb_t cb);

/* ---- TIM3_CH4 (PB1) PWM ---- */
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);
void gtim_timx_pwm_chy_set(uint16_t ccr);

/* ---- TIM5_CH1 (PA0) input capture, 1 tick = 1 us ---- */
void gtim_timx_cap_chy_init(uint32_t arr, uint16_t psc);
gtim_cap_state_t gtim_timx_cap_chy_state(void);
uint32_t gtim_timx_cap_chy_value(void);
void gtim_timx_cap_chy_clear(void);

/* ---- TIM2_CH1 (PA0) external pulse counter ---- */
void gtim_timx_cnt_chy_init(uint16_t psc);
uint32_t gtim_timx_cnt_chy_get_count(void);
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
