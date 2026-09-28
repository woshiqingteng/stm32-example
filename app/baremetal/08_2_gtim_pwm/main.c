/**
 * @file    main.c
 * @brief   08_2_gtim_pwm: TIM3_CH4 (PB1) PWM breathing LED.
 */

#include <stdio.h>

#include "bsp.h"
#include "gtim.h"

#define GTIM_PWM_ARR        500U
#define GTIM_PWM_PSC        90U
/* 90 MHz / (90 * 500) = 2 kHz; duty 0..300/500 = 0..60% */
#define GTIM_PWM_DUTY_MAX_TICK   300U
#define GTIM_PWM_DUTY_STEP_TICK  1U
#define GTIM_PWM_LOOP_MS   10U

/** @brief PWM duty ramp direction. */
typedef enum
{
    GTIM_PWM_RAMP_UP = 0,
    GTIM_PWM_RAMP_DOWN = 1
} gtim_pwm_ramp_t;

int main(void)
{
    int16_t         duty = 0;
    gtim_pwm_ramp_t ramp = GTIM_PWM_RAMP_UP;

    bsp_init();
    printf(APP_BANNER "\r\n");
    gtim_timx_pwm_chy_init(GTIM_PWM_ARR - 1U, GTIM_PWM_PSC - 1U);

    for (;;)
    {
        duty += (ramp == GTIM_PWM_RAMP_UP) ? (int16_t)GTIM_PWM_DUTY_STEP_TICK
                                           : -(int16_t)GTIM_PWM_DUTY_STEP_TICK;
        if (duty >= (int16_t)GTIM_PWM_DUTY_MAX_TICK)
        {
            duty = (int16_t)GTIM_PWM_DUTY_MAX_TICK;
            ramp = GTIM_PWM_RAMP_DOWN;
        }
        else if (duty <= 0)
        {
            duty = 0;
            ramp = GTIM_PWM_RAMP_UP;
        }

        gtim_timx_pwm_chy_set((uint16_t)duty);

        delay_ms(GTIM_PWM_LOOP_MS);
    }
}
