/**
 * @file    main.c
 * @brief   18_2_sleep: KEY0 enters sleep mode, WK_UP wakes the MCU.
 */

#include <stdio.h>
#include "bsp.h"

/** @brief Cause recorded when the MCU wakes from sleep. */
typedef enum
{
    SLEEP_WAKE_NONE = 0,
    SLEEP_WAKE_WKUP
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

    pwr_register_wkup_hook(wkup_hook);
    pwr_wkup_key_init();
    printf("KEY0: enter sleep mode, WK_UP: wake\r\n");

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            printf("Entering sleep mode...\r\n");
            led_on(LED1);

            pwr_enter_sleep();
            HAL_ResumeTick();

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
        delay_ms(10);
    }
}
