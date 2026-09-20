/**
 * @file    main.c
 * @brief   06_wwdg: window watchdog refreshed from its early-wakeup interrupt.
 */

#include "bsp.h"

static void on_early_wakeup(void)
{
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();
    led_on(LED0);
    delay_ms(300);

    wdg_wwdg_register(on_early_wakeup);
    wwdg_init(0x7F, 0x5F, WWDG_PRESCALER_8);

    led_off(LED0);

    for (;;)
    {
    }
}
