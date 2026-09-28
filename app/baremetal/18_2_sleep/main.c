/**
 * @file    main.c
 * @brief   18_2_sleep: KEY0 enters sleep mode, WK_UP wakes the MCU.
 */

#include <stdio.h>

#include "bsp.h"

#define SLEEP_LOOP_MS 10U
#define WKUP_DEBOUNCE_WAIT_MS 30U

/** @brief Cause recorded when the MCU wakes from sleep. */
typedef enum
{
    SLEEP_WAKE_NONE = 0,
    SLEEP_WAKE_WKUP = 1
} sleep_wake_cause_t;

static volatile sleep_wake_cause_t g_wake_cause = SLEEP_WAKE_NONE;

static void wkup_hook(void)
{
    g_wake_cause = SLEEP_WAKE_WKUP;
}

int main(void)
{
    uint32_t t = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");

    pwr_register_wkup_hook(wkup_hook);
    pwr_wkup_key_init();
    printf("KEY0: enter sleep mode, WK_UP: wake\r\n");

    for (;;)
    {
        exti_poll();

        if (key_scan(false) == KEY0)
        {
            printf("Entering sleep mode...\r\n");
            led_on(LED1);

            pwr_enter_sleep();

            /* let the latched WK_UP edge settle past the 20 ms debounce */
            delay_ms(WKUP_DEBOUNCE_WAIT_MS);
            exti_poll();

            led_off(LED1);
            if (g_wake_cause == SLEEP_WAKE_WKUP)
            {
                g_wake_cause = SLEEP_WAKE_NONE;
                printf("Woke from sleep mode (WK_UP)\r\n");
            }
            else
            {
                printf("Woke from sleep mode\r\n");
            }
        }

        if ((++t % 20U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(SLEEP_LOOP_MS);
    }
}
