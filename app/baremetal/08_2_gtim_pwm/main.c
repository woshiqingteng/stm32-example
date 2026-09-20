/**
 * @file    main.c
 * @brief   08_2_gtim_pwm: TIM3_CH4 (PB1) PWM breathing LED.
 */

#include "bsp.h"

int main(void)
{
    int16_t duty = 0;
    int16_t dir = 1;

    bsp_init();
    gtim_timx_pwm_chy_init(500 - 1, 90 - 1);

    for (;;)
    {
        delay_ms(10);

        duty += dir;
        if (duty > 300)
        {
            duty = 300;
            dir = -1;
        }
        else if (duty == 0)
        {
            dir = 1;
        }

        gtim_timx_pwm_chy_set((uint16_t)duty);
    }
}
