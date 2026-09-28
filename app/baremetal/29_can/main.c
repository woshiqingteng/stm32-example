/**
 * @file    main.c
 * @brief   29_can: bxCAN1 loopback test. An 8-byte counter frame is sent on
 *          standard ID 0x12 and the looped-back frame is received and checked.
 *          500 kbps, CAN_MODE_LOOPBACK.
 */

#include <stdio.h>

#include "bsp.h"
#include "can.h"

#define CAN_TEST_ID         0x12U
#define CAN_TEST_LEN_BYTE        8U
#define CAN_PERIOD_MS       500U

static uint32_t g_can_mode = CAN_MODE_LOOPBACK;

static uint8_t can_reinit(void)
{
    return can_init(CAN_SJW_1TQ, CAN_BS2_6TQ, CAN_BS1_8TQ, 6U, g_can_mode);
}

int main(void)
{
    uint8_t txbuf[CAN_TEST_LEN_BYTE];
    uint8_t rxbuf[CAN_TEST_LEN_BYTE];
    uint8_t count = 0U;
    uint8_t i;
    uint8_t rxlen;

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (can_reinit() != 0U)
    {
        printf("CAN init failed!\r\n");
        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    printf("29_can ready (%s), WKUP toggles mode\r\n",
           (g_can_mode == CAN_MODE_LOOPBACK) ? "loopback" : "normal");

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY_WKUP)
        {
            g_can_mode = (g_can_mode == CAN_MODE_LOOPBACK) ? CAN_MODE_NORMAL : CAN_MODE_LOOPBACK;

            if (can_reinit() == 0U)
            {
                printf("CAN mode: %s\r\n",
                       (g_can_mode == CAN_MODE_LOOPBACK) ? "loopback" : "normal");
            }
            else
            {
                printf("CAN mode switch failed\r\n");
            }
        }

        for (i = 0U; i < CAN_TEST_LEN_BYTE; i++)
        {
            txbuf[i] = (uint8_t)(count + i);
        }

        if (can_send(CAN_TEST_ID, txbuf, CAN_TEST_LEN_BYTE) != 0U)
        {
            printf("CAN TX failed\r\n");
        }
        else
        {
            rxlen = can_receive(CAN_TEST_ID, rxbuf);
            printf("TX %02X... RX len:%u\r\n", txbuf[0], (unsigned)rxlen);

            if (rxlen == CAN_TEST_LEN_BYTE)
            {
                printf("RX: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                       rxbuf[0], rxbuf[1], rxbuf[2], rxbuf[3],
                       rxbuf[4], rxbuf[5], rxbuf[6], rxbuf[7]);
            }
        }

        count++;
        led_toggle(LED0);
        delay_ms(CAN_PERIOD_MS);
    }
}
