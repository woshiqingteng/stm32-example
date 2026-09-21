/**
 * @file    main.c
 * @brief   08_2_gtim_pwm: TIM3_CH4 (PB1) PWM breathing LED.
 */

#include "bsp.h"

#define GTIM_PWM_ARR        500U
#define GTIM_PWM_PSC        90U
#define GTIM_PWM_DUTY_MAX   300
#define GTIM_PWM_DUTY_STEP  1
#define GTIM_PWM_DIR_UP     1
#define GTIM_PWM_DIR_DOWN   (-1)
#define GTIM_PWM_DELAY_MS   10U

int main(void)
{
    int16_t duty = 0;
    int16_t dir = GTIM_PWM_DIR_UP;

    bsp_init();
    gtim_timx_pwm_chy_init(GTIM_PWM_ARR - 1U, GTIM_PWM_PSC - 1U);

    for (;;)
    {
        delay_ms(GTIM_PWM_DELAY_MS);

        duty += (int16_t)GTIM_PWM_DUTY_STEP * dir;
        if (duty > GTIM_PWM_DUTY_MAX)
        {
            duty = GTIM_PWM_DUTY_MAX;
            dir = GTIM_PWM_DIR_DOWN;
        }
        else if (duty == 0)
        {
            dir = GTIM_PWM_DIR_UP;
        }

        gtim_timx_pwm_chy_set((uint16_t)duty);
    }
}
