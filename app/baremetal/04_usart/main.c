/**
 * @file    main.c
 * @brief   04_usart: echo USART1 lines received via interrupt.
 */

#include <stdio.h>

#include "bsp.h"
#define USART_BANNER_PERIOD 5000U
#define USART_PROMPT_PERIOD 200U
#define USART_BLINK_PERIOD  30U
#define USART_POLL_MS       10U

int main(void)
{
    uint32_t count = 0;

    bsp_init();
    printf(APP_BANNER "\r\n");

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
        if ((count % USART_BANNER_PERIOD) == 0U)
        {
            printf("\r\nALIENTEK STM32F4 04_usart example\r\n");
            printf("ALIENTEK@STM32F429\r\n\r\n\r\n");
        }
        if ((count % USART_PROMPT_PERIOD) == 0U)
        {
            printf("please input a line ending with CRLF\r\n");
        }
        if ((count % USART_BLINK_PERIOD) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(USART_POLL_MS);
    }
}
