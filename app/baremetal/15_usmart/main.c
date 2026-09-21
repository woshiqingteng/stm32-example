/**
 * @file    main.c
 * @brief   15_usmart: USMART serial console (TIM4 1 us time base, USART1 input).
 */

#include <stdio.h>
#include "bsp.h"
#include "usmart/usmart.h"

#define USMART_APP_BLINK_MS   200U
#define USMART_APP_HZ_PER_MHZ 1000000U

int main(void)
{
    bsp_init();
    usmart_init((uint16_t)(sys_clk_get_hz() / USMART_APP_HZ_PER_MHZ));

    printf("\r\n15_usmart ready. Type 'help' for commands.\r\n");

    for (;;)
    {
        usmart_scan();
        led_toggle(LED0);
        delay_ms(USMART_APP_BLINK_MS);
    }
}
