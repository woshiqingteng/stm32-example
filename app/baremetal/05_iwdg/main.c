/**
 * @file    main.c
 * @brief   05_iwdg: feed the independent watchdog with WK_UP.
 */

#include <stdio.h>

#include "bsp.h"
#include "wdg.h"

#define IWDG_START_DELAY_MS  100U
#define IWDG_LOOP_MS          10U

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    delay_ms(IWDG_START_DELAY_MS);
    iwdg_init();
    led_on(LED0);

    for (;;)
    {
        if (key_scan(true) == KEY_WKUP)
        {
            iwdg_feed();
        }

        delay_ms(IWDG_LOOP_MS);
    }
}
