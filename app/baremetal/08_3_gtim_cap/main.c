/**
 * @file    main.c
 * @brief   08_3_gtim_cap: TIM5_CH1 (PA0) capture, print the high-level width.
 */

#include <stdio.h>
#include "bsp.h"

#define GTIM_CAP_ARR     0xFFFFU
#define GTIM_CAP_PSC     90U
#define GTIM_CAP_LOOP_MS 200U

int main(void)
{
    bsp_init();
    gtim_timx_cap_chy_init(GTIM_CAP_ARR, GTIM_CAP_PSC - 1U);

    for (;;)
    {
        if (gtim_timx_cap_chy_state() == GTIM_CAP_DONE)
        {
            printf("HIGH:%lu us\r\n", (unsigned long)gtim_timx_cap_chy_value());
            gtim_timx_cap_chy_clear();
        }

        led_toggle(LED0);
        delay_ms(GTIM_CAP_LOOP_MS);
    }
}
