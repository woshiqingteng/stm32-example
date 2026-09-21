/**
 * @file    main.c
 * @brief   09_2_atim_oc: TIM8 CH1..4 (PC6..PC9) output-compare toggle.
 */

#include "stm32f4xx_hal.h"
#include "bsp.h"

#define ATIM_OC_ARR     1000U
#define ATIM_OC_PSC     180U
#define ATIM_OC_CCR_CH1 250U
#define ATIM_OC_CCR_CH2 500U
#define ATIM_OC_CCR_CH3 750U
#define ATIM_OC_CCR_CH4 1000U
#define ATIM_OC_LOOP_MS 500U

int main(void)
{
    bsp_init();
    atim_timx_comp_pwm_init(ATIM_OC_ARR - 1U, ATIM_OC_PSC - 1U);

    atim_timx_comp_pwm_set(TIM_CHANNEL_1, ATIM_OC_CCR_CH1 - 1U);
    atim_timx_comp_pwm_set(TIM_CHANNEL_2, ATIM_OC_CCR_CH2 - 1U);
    atim_timx_comp_pwm_set(TIM_CHANNEL_3, ATIM_OC_CCR_CH3 - 1U);
    atim_timx_comp_pwm_set(TIM_CHANNEL_4, ATIM_OC_CCR_CH4 - 1U);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(ATIM_OC_LOOP_MS);
    }
}
