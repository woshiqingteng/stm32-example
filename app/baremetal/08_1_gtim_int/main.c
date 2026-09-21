/**
 * @file    main.c
 * @brief   08_1_gtim_int: TIM3 500 ms interrupt toggles LED1.
 */

#include "bsp.h"

#define GTIM_ARR_500MS 5000U
#define GTIM_PSC_500MS 9000U
#define GTIM_LOOP_MS   200U

static void on_tim3(void)
{
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();

    gtim_timx_int_register(on_tim3);
    gtim_timx_int_init(GTIM_ARR_500MS - 1U, GTIM_PSC_500MS - 1U);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(GTIM_LOOP_MS);
    }
}
