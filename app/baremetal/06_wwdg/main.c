/**
 * @file    main.c
 * @brief   06_wwdg: window watchdog refreshed from its early-wakeup interrupt.
 *          The visible LED1 blink is divided down so it is clearly observable.
 */

#include <stdio.h>

#include "bsp.h"

#define WWDG_START_DELAY_MS   300U
#define WWDG_LED_TOGGLE_DIV   10U   /*!< toggle LED1 every 10 early-wakeup interrupts */

static void on_early_wakeup(void)
{
    static uint8_t count;

    if (++count >= WWDG_LED_TOGGLE_DIV)
    {
        count = 0U;
        led_toggle(LED1);
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    led_on(LED0);
    delay_ms(WWDG_START_DELAY_MS);

    wdg_wwdg_register(on_early_wakeup);
    wwdg_init();

    led_off(LED0);

    for (;;)
    {
    }
}
