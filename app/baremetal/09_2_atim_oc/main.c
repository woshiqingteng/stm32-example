/**
 * @file    main.c
 * @brief   09_2_atim_oc: TIM8 CH1..4 (PC6..PC9) output-compare toggle.
 */

#include "stm32f4xx_hal.h"
#include "bsp.h"

int main(void)
{
    bsp_init();
    atim_timx_comp_pwm_init(1000 - 1, 180 - 1);

    atim_timx_comp_pwm_set(TIM_CHANNEL_1, 250 - 1);
    atim_timx_comp_pwm_set(TIM_CHANNEL_2, 500 - 1);
    atim_timx_comp_pwm_set(TIM_CHANNEL_3, 750 - 1);
    atim_timx_comp_pwm_set(TIM_CHANNEL_4, 1000 - 1);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500);
    }
}
