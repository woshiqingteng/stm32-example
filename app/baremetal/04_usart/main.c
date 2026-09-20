/**
 * @file    main.c
 * @brief   04_usart: echo USART1 lines received via interrupt.
 */

#include <stdio.h>
#include "bsp.h"

int main(void)
{
    uint32_t count = 0;

    bsp_init();
    printf("04_usart ready\r\n");

    for (;;)
    {
        if (usart_rx_state() == USART_RX_READY)
        {
            uint16_t len = usart_rx_len();
            const uint8_t *buf = usart_rx_buf();

            printf("recv %u bytes: %.*s\r\n", (unsigned)len, (int)len, (const char *)buf);
            usart_rx_clear();
        }

        count++;
        if ((count % 200U) == 0U)
        {
            printf("please input a line ending with CRLF\r\n");
        }
        if ((count % 20U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(10);
    }
}
