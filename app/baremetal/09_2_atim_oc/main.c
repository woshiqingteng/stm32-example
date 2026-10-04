/**
 * @file    main.c
 * @brief   09_2_atim_oc: TIM8 CH1..4 (PC6..PC9) output-compare toggle.
 */

#include <stdio.h>

#include "bsp.h"
#include "tim.h"

#define ATIM_OC_ARR     1000U
#define ATIM_OC_PSC     180U
/* Counter: 180 MHz / (180 * 1000) = 1 kHz. TOGGLE output: 500 Hz, 50% duty;
 * CCRx sets the phase: CH1..4 = 25/50/75/100%. */
#define ATIM_OC_CCR_CH1 250U
#define ATIM_OC_CCR_CH2 500U
#define ATIM_OC_CCR_CH3 750U
#define ATIM_OC_CCR_CH4 1000U
#define ATIM_OC_LOOP_MS 500U

int main(void)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    bsp_init();
    printf(APP_BANNER "\r\n");

    cfg.id       = TIM_ID_8;
    cfg.mode     = TIM_MODE_OC;
    cfg.channel  = TIM_CH1;
    cfg.polarity = TIM_POL_HIGH;
    cfg.pull     = TIM_PULL_NONE;
    cfg.arr      = ATIM_OC_ARR - 1U;
    cfg.psc      = ATIM_OC_PSC - 1U;
    tim_init(&cfg);

    tim_set(TIM_ID_8, TIM_CH1, TIM_PARAM_CCR, ATIM_OC_CCR_CH1 - 1U);
    tim_set(TIM_ID_8, TIM_CH2, TIM_PARAM_CCR, ATIM_OC_CCR_CH2 - 1U);
    tim_set(TIM_ID_8, TIM_CH3, TIM_PARAM_CCR, ATIM_OC_CCR_CH3 - 1U);
    tim_set(TIM_ID_8, TIM_CH4, TIM_PARAM_CCR, ATIM_OC_CCR_CH4 - 1U);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(ATIM_OC_LOOP_MS);
    }
}
