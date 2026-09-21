/**
 * @file    main.c
 * @brief   06_wwdg: window watchdog refreshed from its early-wakeup interrupt.
 */

#include "bsp.h"

#define WWDG_COUNTER        0x7FU
#define WWDG_WINDOW         0x5FU
#define WWDG_START_DELAY_MS 300U

static void on_early_wakeup(void)
{
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();
    led_on(LED0);
    delay_ms(WWDG_START_DELAY_MS);

    wdg_wwdg_register(on_early_wakeup);
    wwdg_init(WWDG_COUNTER, WWDG_WINDOW, WWDG_PRESCALER_8);

    led_off(LED0);

    for (;;)
    {
    }
}
