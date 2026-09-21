/**
 * @file    main.c
 * @brief   18_2_sleep: KEY0 enters sleep mode, WK_UP wakes the MCU.
 */

#include <stdio.h>
#include "bsp.h"

static volatile bool g_wkup_seen;

static void wkup_hook(void)
{
    g_wkup_seen = true;
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
            if (g_wkup_seen)
            {
                g_wkup_seen = false;
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
