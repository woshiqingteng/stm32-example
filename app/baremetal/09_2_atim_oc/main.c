/**
 * @file    main.c
 * @brief   09_2_atim_oc: TIM8 CH1..4 (PC6..PC9) output-compare toggle.
 */

#include <stdio.h>
#include "bsp.h"
#define ATIM_OC_ARR_TICK     1000U
#define ATIM_OC_PSC_DIV     180U
#define ATIM_OC_CCR_CH1_TICK 250U
#define ATIM_OC_CCR_CH2_TICK 500U
#define ATIM_OC_CCR_CH3_TICK 750U
#define ATIM_OC_CCR_CH4_TICK 1000U
#define ATIM_OC_LOOP_MS 500U

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    atim_timx_comp_pwm_init(ATIM_OC_ARR_TICK - 1U, ATIM_OC_PSC_DIV - 1U);

    atim_timx_comp_pwm_set(ATIM_CH1, ATIM_OC_CCR_CH1_TICK - 1U);
    atim_timx_comp_pwm_set(ATIM_CH2, ATIM_OC_CCR_CH2_TICK - 1U);
    atim_timx_comp_pwm_set(ATIM_CH3, ATIM_OC_CCR_CH3_TICK - 1U);
    atim_timx_comp_pwm_set(ATIM_CH4, ATIM_OC_CCR_CH4_TICK - 1U);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(ATIM_OC_LOOP_MS);
    }
}
