/**
 * @file    main.c
 * @brief   09_3_atim_cplm: TIM1 complementary PWM with dead time (PE9/PE8).
 */

#include "bsp.h"

int main(void)
{
    bsp_init();
    atim_timx_cplm_pwm_init(1000 - 1, 180 - 1);
    atim_timx_cplm_pwm_set(300, 100);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500);
    }
}
