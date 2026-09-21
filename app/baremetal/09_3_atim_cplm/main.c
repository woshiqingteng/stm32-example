/**
 * @file    main.c
 * @brief   09_3_atim_cplm: TIM1 complementary PWM with dead time (PE9/PE8).
 */

#include "bsp.h"

#define ATIM_CPLM_ARR     1000U
#define ATIM_CPLM_PSC     180U
#define ATIM_CPLM_CCR     300U
#define ATIM_CPLM_DTG     100U
#define ATIM_CPLM_LOOP_MS 500U

int main(void)
{
    bsp_init();
    atim_timx_cplm_pwm_init(ATIM_CPLM_ARR - 1U, ATIM_CPLM_PSC - 1U);
    atim_timx_cplm_pwm_set(ATIM_CPLM_CCR, ATIM_CPLM_DTG);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(ATIM_CPLM_LOOP_MS);
    }
}
