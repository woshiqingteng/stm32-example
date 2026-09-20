/**
 * @file    main.c
 * @brief   02_key: scan keys and toggle LEDs.
 */

#include <stdio.h>
#include "bsp.h"

int main(void)
{
    bool led0_on = true;
    bool led1_on = false;

    bsp_init();
    led_on(LED0);

    for (;;)
    {
        key_id_t key = key_scan(false);

        switch (key)
        {
            case KEY_WKUP:
                led1_on = !led1_on;
                led0_on = led1_on;
                break;
            case KEY0:
                led0_on = !led0_on;
                led1_on = !led1_on;
                break;
            case KEY1:
                led1_on = !led1_on;
                break;
            case KEY2:
                led0_on = !led0_on;
                break;
            default:
                delay_ms(10);
                break;
        }

        if (led0_on) { led_on(LED0); } else { led_off(LED0); }
        if (led1_on) { led_on(LED1); } else { led_off(LED1); }
    }
}
