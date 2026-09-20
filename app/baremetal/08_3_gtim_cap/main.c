/**
 * @file    main.c
 * @brief   08_3_gtim_cap: TIM5_CH1 (PA0) capture, print the high-level width.
 */

#include <stdio.h>
#include "bsp.h"

int main(void)
{
    bsp_init();
    gtim_timx_cap_chy_init(0xFFFF, 90 - 1);

    for (;;)
    {
        if (gtim_timx_cap_chy_state() == GTIM_CAP_DONE)
        {
            printf("HIGH:%lu us\r\n", (unsigned long)gtim_timx_cap_chy_value());
            gtim_timx_cap_chy_clear();
        }

        led_toggle(LED0);
        delay_ms(200);
    }
}
