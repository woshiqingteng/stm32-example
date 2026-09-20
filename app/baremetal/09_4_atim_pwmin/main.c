/**
 * @file    main.c
 * @brief   09_4_atim_pwmin: TIM3_CH4 (PB1) test PWM measured by TIM8 PWM input
 *          (jumper PB1 -> PC6), print frequency.
 */

#include <stdio.h>
#include "bsp.h"

int main(void)
{
    uint32_t blink = 0;

    bsp_init();

    gtim_timx_pwm_chy_init(10 - 1, 90 - 1); /* 100 kHz test signal */
    gtim_timx_pwm_chy_set(2);

    atim_timx_pwmin_chy_init();

    for (;;)
    {
        if ((blink % 20U) == 0U)
        {
            if (atim_timx_pwmin_chy_state() == ATIM_PWMIN_DONE)
            {
                uint16_t psc   = atim_timx_pwmin_chy_psc();
                uint32_t hval  = atim_timx_pwmin_chy_hval();
                uint32_t cval  = atim_timx_pwmin_chy_cval();
                uint32_t scale = (uint32_t)psc + 1U;
                uint32_t htime = (hval * scale) / 180U;
                uint32_t ctime = (cval * scale) / 180U;
                uint32_t freq  = (ctime != 0U) ? (1000000U / ctime) : 0U;

                printf("psc:%u hval:%lu cval:%lu high:%luus cycle:%luus freq:%luHz\r\n",
                       (unsigned)psc, (unsigned long)hval, (unsigned long)cval,
                       (unsigned long)htime, (unsigned long)ctime, (unsigned long)freq);

                atim_timx_pwmin_chy_restart();
            }

            led_toggle(LED1);
        }

        blink++;
        delay_ms(10);
    }
}
