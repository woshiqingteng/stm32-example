/**
 * @file    main.c
 * @brief   05_iwdg: feed the independent watchdog with WK_UP.
 */

#include "bsp.h"

#define IWDG_RELOAD          500U
#define IWDG_START_DELAY_MS  100U
#define KEY_POLL_MS          10U

int main(void)
{
    bsp_init();
    delay_ms(IWDG_START_DELAY_MS);
    iwdg_init(IWDG_PRESCALER_64, IWDG_RELOAD);
    led_on(LED0);

    for (;;)
    {
        if (key_scan(true) == KEY_WKUP)
        {
            iwdg_feed();
        }
        delay_ms(KEY_POLL_MS);
    }
}
