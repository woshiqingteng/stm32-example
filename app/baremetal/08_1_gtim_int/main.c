/**
 * @file    main.c
 * @brief   08_1_gtim_int: TIM3 500 ms interrupt toggles LED1.
 */

#include "bsp.h"

static void on_tim3(void)
{
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();

    gtim_timx_int_register(on_tim3);
    gtim_timx_int_init(5000 - 1, 9000 - 1);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(200);
    }
}
