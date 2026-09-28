/**
 * @file    main.c
 * @brief   06_wwdg: window watchdog refreshed from its early-wakeup interrupt.
 */

#include <stdio.h>

#include "bsp.h"

#define WWDG_COUNTER_TICK        0x7FU
#define WWDG_WINDOW_TICK         0x5FU
#define WWDG_START_DELAY_MS 300U

static void on_early_wakeup(void)
{
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    led_on(LED0);
    delay_ms(WWDG_START_DELAY_MS);

    wdg_wwdg_register(on_early_wakeup);
    wwdg_init(WWDG_COUNTER_TICK, WWDG_WINDOW_TICK, WWDG_PRESCALER_8);

    led_off(LED0);

    for (;;)
    {
    }
}
