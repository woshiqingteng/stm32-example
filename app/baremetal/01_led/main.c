/**
 * @file    main.c
 * @brief   01_led: alternate LED0/LED1 every 500ms.
 */

#include <stdio.h>

#include "bsp.h"
#define LED_BLINK_INTERVAL_MS 500U

int main(void)
{
    bsp_init();

    printf("01_led ready\r\n");

    for (;;)
    {
        led_on(LED0);
        led_off(LED1);
        printf("LED0 on, LED1 off\r\n");
        delay_ms(LED_BLINK_INTERVAL_MS);

        led_off(LED0);
        led_on(LED1);
        printf("LED0 off, LED1 on\r\n");
        delay_ms(LED_BLINK_INTERVAL_MS);
    }
}
