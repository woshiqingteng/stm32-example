/**
 * @file    main.c
 * @brief   02_key: scan keys and toggle LEDs.
 */

#include <stdio.h>
#include "bsp.h"

#define KEY_IDLE_POLL_MS 10U

typedef enum
{
    LED_STATE_OFF = 0,
    LED_STATE_ON,
} led_state_t;

static led_state_t led_state_toggle(led_state_t state)
{
    return (state == LED_STATE_ON) ? LED_STATE_OFF : LED_STATE_ON;
}

int main(void)
{
    led_state_t led0_state = LED_STATE_ON;
    led_state_t led1_state = LED_STATE_OFF;

    bsp_init();
    led_on(LED0);

    for (;;)
    {
        key_id_t key = key_scan(false);

        switch (key)
        {
            case KEY_WKUP:
                led1_state = led_state_toggle(led1_state);
                led0_state = led1_state;
                break;
            case KEY0:
                led0_state = led_state_toggle(led0_state);
                led1_state = led_state_toggle(led1_state);
                break;
            case KEY1:
                led1_state = led_state_toggle(led1_state);
                break;
            case KEY2:
                led0_state = led_state_toggle(led0_state);
                break;
            default:
                delay_ms(KEY_IDLE_POLL_MS);
                break;
        }

        if (led0_state == LED_STATE_ON) { led_on(LED0); } else { led_off(LED0); }
        if (led1_state == LED_STATE_ON) { led_on(LED1); } else { led_off(LED1); }
    }
}
