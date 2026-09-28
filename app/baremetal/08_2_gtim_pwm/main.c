/**
 * @file    main.c
 * @brief   08_2_gtim_pwm: TIM3_CH4 (PB1) PWM breathing LED.
 */

#include <stdio.h>

#include "bsp.h"

#define GTIM_PWM_ARR_TICK        500U
#define GTIM_PWM_PSC_DIV        90U
#define GTIM_PWM_DUTY_MAX_TICK   300
#define GTIM_PWM_DUTY_STEP_TICK  1
#define GTIM_PWM_DELAY_MS   10U

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
    gtim_timx_pwm_chy_init(GTIM_PWM_ARR_TICK - 1U, GTIM_PWM_PSC_DIV - 1U);

    for (;;)
    {
        delay_ms(GTIM_PWM_DELAY_MS);

        duty += (ramp == GTIM_PWM_RAMP_UP) ? (int16_t)GTIM_PWM_DUTY_STEP_TICK
                                           : -(int16_t)GTIM_PWM_DUTY_STEP_TICK;
        if (duty > GTIM_PWM_DUTY_MAX_TICK)
        {
            duty = GTIM_PWM_DUTY_MAX_TICK;
            ramp = GTIM_PWM_RAMP_DOWN;
        }
        else if (duty == 0)
        {
            ramp = GTIM_PWM_RAMP_UP;
        }

        gtim_timx_pwm_chy_set((uint16_t)duty);
    }
}
