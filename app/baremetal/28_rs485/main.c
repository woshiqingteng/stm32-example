/**
 * @file    main.c
 * @brief   28_rs485: RS485 half-duplex test on USART2. A counter-tagged string
 *          is transmitted periodically and any bytes received (for example via
 *          an external loopback) are printed over USART1.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"

#define RS485_BAUDRATE      9600U
#define RS485_PERIOD_MS     500U

#define RX_MAX              32U

int main(void)
{
    uint8_t  txbuf[16];
    uint8_t  rxbuf[RX_MAX];
    uint16_t rxlen;
    uint32_t count = 0U;
    uint8_t  i;

    bsp_init();
    rs485_init(RS485_BAUDRATE);

    printf("28_rs485 ready\r\n");

    for (;;)
    {
        (void)snprintf((char *)txbuf, sizeof(txbuf), "RS485 #%lu\r\n", (unsigned long)count);
        printf("TX: %s", (char *)txbuf);
        rs485_send(txbuf, (uint16_t)strlen((char *)txbuf));

        rxlen = rs485_receive(rxbuf, RX_MAX);

        if (rxlen > 0U)
        {
            char line[48];

            rxbuf[(rxlen < RX_MAX) ? rxlen : (RX_MAX - 1U)] = '\0';
            for (i = 0U; (i < rxlen) && (i < (RX_MAX - 1U)); i++)
            {
                if ((rxbuf[i] < 0x20U) || (rxbuf[i] > 0x7EU))
                {
                    rxbuf[i] = '.';
                }
            }
            sprintf(line, "RX(%u): %s", (unsigned)rxlen, (char *)rxbuf);
            printf("%s\r\n", line);
        }
        else
        {
            printf("RX: none\r\n");
        }

        count++;
        led_toggle(LED0);
        delay_ms(RS485_PERIOD_MS);
    }
}
