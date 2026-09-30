/**
 * @file    main.c
 * @brief   36_spi_wireless: NRF24L01 wireless link test. The role (RX/TX) can be
 *          selected with KEY0 / KEY1; when no key is pressed the default is RX.
 *          The transmitted / received payload is printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "wireless.h"

#define NRF_MODE_SELECT_MS  5000U
#define NRF_TX_PERIOD_MS    500U

/** @brief  Selected NRF24L01 role. */
typedef enum
{
    NRF_MODE_RX = 0,
    NRF_MODE_TX = 1
} nrf_mode_t;

static void nrf_run_rx(void)
{
    uint8_t payload[WIRELESS_PLOAD_WIDTH];

    printf("NRF24L01 RX mode\r\n");
    wireless_rx_mode();

    for (;;)
    {
        if (wireless_rx_packet(payload) == 0U)
        {
            payload[WIRELESS_PLOAD_WIDTH - 1U] = '\0';
            printf("RX: %s\r\n", (char *)payload);
            led_toggle(LED1);
        }
        else
        {
            delay_us(100U);
        }
    }
}

static void nrf_run_tx(void)
{
    uint8_t  payload[WIRELESS_PLOAD_WIDTH];
    uint32_t count = 0U;
    uint8_t  i;

    printf("NRF24L01 TX mode\r\n");
    wireless_tx_mode();

    for (;;)
    {
        for (i = 0U; i < WIRELESS_PLOAD_WIDTH; i++)
        {
            payload[i] = (uint8_t)' ';
        }

        (void)snprintf((char *)payload, WIRELESS_PLOAD_WIDTH, "NRF TX #%lu",
                       (unsigned long)count);

        if (wireless_tx_packet(payload) == 0U)
        {
            printf("TX: %s\r\n", (char *)payload);
            led_toggle(LED0);
        }
        else
        {
            printf("NRF24L01 TX failed\r\n");
        }

        count++;
        delay_ms(NRF_TX_PERIOD_MS);
    }
}

int main(void)
{
    uint8_t  key;
    uint32_t waited = 0U;
    nrf_mode_t mode = NRF_MODE_RX; /* RX by default */

    bsp_init();
    wireless_init();

    while (wireless_check() != 0U)
    {
        printf("NRF24L01 not found!\r\n");
        delay_ms(200U);
    }

    printf(APP_BANNER "\r\n");
    printf("KEY0:RX  KEY1:TX\r\n");

    while (waited < NRF_MODE_SELECT_MS)
    {
        key = key_scan(false);

        if (key == KEY0)
        {
            mode = NRF_MODE_RX;
            break;
        }
        else if (key == KEY1)
        {
            mode = NRF_MODE_TX;
            break;
        }

        waited += 10U;
        delay_ms(10U);
    }

    if (mode == NRF_MODE_RX)
    {
        nrf_run_rx();
    }
    else
    {
        nrf_run_tx();
    }

    return 0;
}
