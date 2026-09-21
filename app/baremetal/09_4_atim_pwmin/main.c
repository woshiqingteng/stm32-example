/**
 * @file    main.c
 * @brief   09_4_atim_pwmin: TIM3_CH4 (PB1) test PWM measured by TIM8 PWM input
 *          (jumper PB1 -> PC6), print frequency.
 */

#include <stdio.h>
#include "bsp.h"

#define ATIM_PWMIN_TEST_ARR      10U
#define ATIM_PWMIN_TEST_PSC      90U
#define ATIM_PWMIN_TEST_CCR      2U
#define ATIM_PWMIN_TIMER_CLK_MHZ 180U
#define ATIM_PWMIN_US_PER_SECOND 1000000U
#define ATIM_PWMIN_BLINK_DIV     20U
#define ATIM_PWMIN_LOOP_MS       10U

int main(void)
{
    uint32_t blink = 0;

    bsp_init();

    gtim_timx_pwm_chy_init(ATIM_PWMIN_TEST_ARR - 1U, ATIM_PWMIN_TEST_PSC - 1U);
    gtim_timx_pwm_chy_set(ATIM_PWMIN_TEST_CCR);

    atim_timx_pwmin_chy_init();

    for (;;)
    {
        if ((blink % ATIM_PWMIN_BLINK_DIV) == 0U)
        {
            if (atim_timx_pwmin_chy_state() == ATIM_PWMIN_DONE)
            {
                uint16_t psc   = atim_timx_pwmin_chy_psc();
                uint32_t hval  = atim_timx_pwmin_chy_hval();
                uint32_t cval  = atim_timx_pwmin_chy_cval();
                uint32_t scale = (uint32_t)psc + 1U;
                uint32_t htime = (hval * scale) / ATIM_PWMIN_TIMER_CLK_MHZ;
                uint32_t ctime = (cval * scale) / ATIM_PWMIN_TIMER_CLK_MHZ;
                uint32_t freq  = (ctime != 0U) ? (ATIM_PWMIN_US_PER_SECOND / ctime) : 0U;

                printf("psc:%u hval:%lu cval:%lu high:%luus cycle:%luus freq:%luHz\r\n",
                       (unsigned)psc, (unsigned long)hval, (unsigned long)cval,
                       (unsigned long)htime, (unsigned long)ctime, (unsigned long)freq);

                atim_timx_pwmin_chy_restart();
            }

            led_toggle(LED1);
        }

        blink++;
        delay_ms(ATIM_PWMIN_LOOP_MS);
    }
}
