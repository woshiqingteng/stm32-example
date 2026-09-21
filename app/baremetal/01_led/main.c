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

    printf("01_led: STM32F429IGTx, HCLK = %lu Hz\r\n", (unsigned long)sys_clk_get_hz());

    for (;;)
    {
        led_on(LED0);
        led_off(LED1);
        delay_ms(LED_BLINK_INTERVAL_MS);

        led_off(LED0);
        led_on(LED1);
        delay_ms(LED_BLINK_INTERVAL_MS);
    }
}
