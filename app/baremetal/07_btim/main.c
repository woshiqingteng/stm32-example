/**
 * @file    main.c
 * @brief   07_btim: TIM6 500 ms interrupt toggles LED1.
 */

#include "bsp.h"

#define BTIM_ARR_500MS 5000U
#define BTIM_PSC_500MS 9000U
#define BTIM_LOOP_MS   200U

static void on_tim6(void)
{
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();

    btim_timx_int_register(on_tim6);
    btim_timx_int_init(BTIM_ARR_500MS - 1U, BTIM_PSC_500MS - 1U);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BTIM_LOOP_MS);
    }
}
