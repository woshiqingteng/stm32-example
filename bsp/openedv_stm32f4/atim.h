/**
 * @file    atim.h
 * @brief   Advanced timer (TIM8 / TIM1) interface: NPWM / output-compare /
 *          complementary PWM / PWM input.
 */

#ifndef BSP_ATIM_H
#define BSP_ATIM_H

#include <stdint.h>

/** @brief TIM8 output-compare channels CH1..CH4 (PC6..PC9). */
typedef enum
{
    ATIM_CH1 = 0,
    ATIM_CH2 = 1,
    ATIM_CH3 = 2,
    ATIM_CH4 = 3,
} atim_channel_t;

/* ---- TIM8_CH1 (PC6): emit a given number of PWM pulses ---- */
void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc);
void atim_timx_npwm_chy_set(uint32_t npwm);

/* ---- TIM8 CH1..4 (PC6..PC9): output-compare toggle ---- */
void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc);
void atim_timx_comp_pwm_set(atim_channel_t channel, uint16_t ccr);

/* ---- TIM1 CH1/CH1N/BKIN (PE9/PE8/PE15): complementary PWM + dead time ---- */
void atim_timx_cplm_pwm_init(uint16_t arr, uint16_t psc);
void atim_timx_cplm_pwm_set(uint16_t ccr, uint8_t dtg);

/* ---- TIM8_CH1 (PC6): PWM input mode ---- */
/*!< Called from the capture ISR when a measurement completes (raw ticks). */
typedef void (*atim_pwmin_cb_t)(uint16_t psc, uint32_t hval, uint32_t cval);

void atim_timx_pwmin_chy_register(atim_pwmin_cb_t cb);
void atim_timx_pwmin_chy_init(void);
void atim_timx_pwmin_chy_restart(void);

#endif /* BSP_ATIM_H */
