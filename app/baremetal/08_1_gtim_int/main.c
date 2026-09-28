/**
 * @file    main.c
 * @brief   08_1_gtim_int: TIM3 500 ms interrupt toggles LED1.
 */

#include <stdio.h>

#include "bsp.h"

#define GTIM_ARR 5000U
#define GTIM_PSC 9000U
#define GTIM_LOOP_MS   200U

static void on_tim3(void)
{
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    gtim_timx_int_register(on_tim3);
    gtim_timx_int_init(GTIM_ARR - 1U, GTIM_PSC - 1U);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(GTIM_LOOP_MS);
    }
}
