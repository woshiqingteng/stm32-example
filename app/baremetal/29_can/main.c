/**
 * @file    main.c
 * @brief   29_can: bxCAN1 loopback test. An 8-byte counter frame is sent on
 *          standard ID 0x12 and the looped-back frame is received and checked.
 *          500 kbps, CAN_MODE_LOOPBACK.
 */

#include <stdio.h>
#include "bsp.h"

#define CAN_TEST_ID         0x12U
#define CAN_TEST_LEN        8U
#define CAN_PERIOD_MS       500U

int main(void)
{
    uint8_t txbuf[CAN_TEST_LEN];
    uint8_t rxbuf[CAN_TEST_LEN];
    uint8_t count = 0U;
    uint8_t i;
    uint8_t rxlen;
    char    line[48];

    bsp_init();

    if (can_init(CAN_SJW_1TQ, CAN_BS2_6TQ, CAN_BS1_8TQ, 6U, CAN_MODE_LOOPBACK) != 0U)
    {
        printf("CAN init failed!\r\n");
        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    printf("29_can ready\r\n");

    for (;;)
    {
        for (i = 0U; i < CAN_TEST_LEN; i++)
        {
            txbuf[i] = (uint8_t)(count + i);
        }

        if (can_send(CAN_TEST_ID, txbuf, CAN_TEST_LEN) != 0U)
        {
            printf("CAN TX failed\r\n");
        }
        else
        {
            rxlen = can_receive(CAN_TEST_ID, rxbuf);
            sprintf(line, "TX %02X... RX len:%u", txbuf[0], (unsigned)rxlen);
            printf("%s\r\n", line);

            if (rxlen == CAN_TEST_LEN)
            {
                sprintf(line, "RX: %02X %02X %02X %02X %02X %02X %02X %02X",
                        rxbuf[0], rxbuf[1], rxbuf[2], rxbuf[3],
                        rxbuf[4], rxbuf[5], rxbuf[6], rxbuf[7]);
                printf("%s\r\n", line);
            }
        }

        count++;
        led_toggle(LED0);
        delay_ms(CAN_PERIOD_MS);
    }
}
