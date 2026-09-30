/**
 * @file    wireless.c
 * @brief   2.4 GHz wireless link (delegates to the chip driver).
 */

#include "wireless.h"
#include "wireless_nrf24l01.h"

/* The functional packet size must match the concrete transceiver width. */
_Static_assert(WIRELESS_PLOAD_WIDTH == NRF24L01_TX_PLOAD_WIDTH,
               "WIRELESS_PLOAD_WIDTH must match NRF24L01_TX_PLOAD_WIDTH");
_Static_assert(WIRELESS_PLOAD_WIDTH == NRF24L01_RX_PLOAD_WIDTH,
               "WIRELESS_PLOAD_WIDTH must match NRF24L01_RX_PLOAD_WIDTH");

void wireless_init(void)
{
    wireless_nrf24l01_init();
}

uint8_t wireless_check(void)
{
    return wireless_nrf24l01_check();
}

void wireless_rx_mode(void)
{
    wireless_nrf24l01_rx_mode();
}

void wireless_tx_mode(void)
{
    wireless_nrf24l01_tx_mode();
}

uint8_t wireless_tx_packet(uint8_t *ptxbuf)
{
    return wireless_nrf24l01_tx_packet(ptxbuf);
}

uint8_t wireless_rx_packet(uint8_t *prxbuf)
{
    return wireless_nrf24l01_rx_packet(prxbuf);
}
