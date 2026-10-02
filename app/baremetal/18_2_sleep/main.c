/**
 * @file    main.c
 * @brief   18_2_sleep: KEY0 enters sleep mode, WK_UP wakes the MCU.
 */

#include <stdio.h>

#include "bsp.h"
#include "exti.h"
#include "pwr.h"

#define SLEEP_LOOP_MS 10U
#define WKUP_DEBOUNCE_WAIT_MS 30U

static volatile bool g_woke_by_wkup;

static void wkup_cb(void)
{
    g_woke_by_wkup = true;
}

int main(void)
{
    uint32_t t = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");

    pwr_wkup_key_init(&wkup_cb);
    printf("KEY0: enter sleep  WKUP: wake\r\n");

    for (;;)
    {
        exti_poll();

        if (key_scan(false) == KEY0)
        {
            printf("Entering sleep mode...\r\n");
            led_on(LED1);

            g_woke_by_wkup = false;          /* clear any stale WK_UP latch */
            pwr_enter_sleep();

            /* let the latched WK_UP edge settle past the 20 ms debounce */
            delay_ms(WKUP_DEBOUNCE_WAIT_MS);
            exti_poll();

            led_off(LED1);
            printf(g_woke_by_wkup ? "Woke from sleep mode (WKUP)\r\n"
                                  : "Woke from sleep mode\r\n");
        }

        if ((++t % 20U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(SLEEP_LOOP_MS);
    }
}
