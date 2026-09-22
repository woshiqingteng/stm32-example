/**
 * @file    nrf24l01.h
 * @brief   NRF24L01 2.4 GHz transceiver on SPI2 (CE=PG12, CSN=PG10, IRQ=PI11).
 */

#ifndef BSP_NRF24L01_H
#define BSP_NRF24L01_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define NRF24L01_CE_GPIO_PORT   GPIOG
#define NRF24L01_CE_GPIO_PIN    GPIO_PIN_12
#define NRF24L01_CSN_GPIO_PORT  GPIOG
#define NRF24L01_CSN_GPIO_PIN   GPIO_PIN_10
#define NRF24L01_IRQ_GPIO_PORT  GPIOI
#define NRF24L01_IRQ_GPIO_PIN   GPIO_PIN_11

#define NRF24L01_TX_ADR_WIDTH   5U
#define NRF24L01_RX_ADR_WIDTH   5U
#define NRF24L01_TX_PLOAD_WIDTH 32U
#define NRF24L01_RX_PLOAD_WIDTH 32U

/** @brief  Configure CE/CSN/IRQ and bring up SPI2. */
void nrf24l01_init(void);

/** @brief  Write/read-back the TX address. @return 0 if the device is present. */
uint8_t nrf24l01_check(void);

/** @brief  Enter receive mode. */
void nrf24l01_rx_mode(void);

/** @brief  Enter transmit mode. */
void nrf24l01_tx_mode(void);

/** @brief  Transmit one fixed-width packet. @return 0 on success. */
uint8_t nrf24l01_tx_packet(uint8_t *ptxbuf);

/** @brief  Fetch one received packet. @return 0 on success. */
uint8_t nrf24l01_rx_packet(uint8_t *prxbuf);

#endif /* BSP_NRF24L01_H */
