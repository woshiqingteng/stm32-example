/**
 * @file    main.c
 * @brief   01_led: alternate LED0/LED1 every 500ms.
 */

#include <stdio.h>
#include "bsp.h"
#include "sys.h"
#include "delay.h"
#include "led.h"

int main(void)
{
    bsp_init();

    printf("01_led: STM32F429IGTx, HCLK = %lu Hz\r\n", (unsigned long)sys_clk_get_hz());

    for (;;)
    {
        led_on(LED0);
        led_off(LED1);
        delay_ms(500);

        led_off(LED0);
        led_on(LED1);
        delay_ms(500);
    }
}
