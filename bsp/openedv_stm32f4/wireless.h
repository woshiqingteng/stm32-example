/**
 * @file    wireless.h
 * @brief   2.4 GHz wireless link: generic packet API.
 *
 * Device-independent API; the concrete transceiver driver sits behind it
 * (wireless_nrf24l01.h).
 */

#ifndef BSP_WIRELESS_H
#define BSP_WIRELESS_H

#include <stdint.h>

/** @brief  Fixed packet payload size in bytes. */
#define WIRELESS_PLOAD_WIDTH 32U

/** @brief  Configure the transceiver. */
void wireless_init(void);

/** @brief  Probe the transceiver. @return 0 if the device is present. */
uint8_t wireless_check(void);

/** @brief  Enter receive mode. */
void wireless_rx_mode(void);

/** @brief  Enter transmit mode. */
void wireless_tx_mode(void);

/** @brief  Transmit one fixed-width packet. @return 0 on success. */
uint8_t wireless_tx_packet(uint8_t *ptxbuf);

/** @brief  Fetch one received packet. @return 0 on success. */
uint8_t wireless_rx_packet(uint8_t *prxbuf);

#endif /* BSP_WIRELESS_H */
