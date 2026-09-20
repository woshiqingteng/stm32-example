/**
 * @file    main.c
 * @brief   05_iwdg: feed the independent watchdog with WK_UP.
 */

#include "bsp.h"

int main(void)
{
    bsp_init();
    delay_ms(100);
    iwdg_init(IWDG_PRESCALER_64, 500);
    led_on(LED0);

    for (;;)
    {
        if (key_scan(true) == KEY_WKUP)
        {
            iwdg_feed();
        }
        delay_ms(10);
    }
}
