/**
 * @file    rs485.h
 * @brief   RS485 half-duplex driver: USART2 (PA2 TX / PA3 RX) with the
 *          direction line driven through the PCF8574 expander (bit RE).
 */

#ifndef BSP_RS485_H
#define BSP_RS485_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define RS485_UX            USART2
#define RS485_UX_IRQn       USART2_IRQn
#define RS485_TX_GPIO_PORT  GPIOA
#define RS485_TX_GPIO_PIN   GPIO_PIN_2
#define RS485_RX_GPIO_PORT  GPIOA
#define RS485_RX_GPIO_PIN   GPIO_PIN_3
#define RS485_GPIO_AF       GPIO_AF7_USART2

#define RS485_REC_LEN       64U

/** @brief Callback invoked for every byte received on USART2. */
typedef void (*rs485_rx_byte_cb_t)(uint8_t byte);

/** @brief  Initialise USART2, the direction expander and RX interrupt. */
void rs485_init(uint32_t baudrate);

/** @brief  Drive the DE/RE line: 1 = transmit, 0 = receive. */
void rs485_tx_set(uint8_t en);

/** @brief  Transmit @p len bytes, switching the transceiver to TX then back to RX. */
void rs485_send(const uint8_t *buf, uint16_t len);

/**
 * @brief  Drain the received bytes once the line has been idle.
 * @param  buf      destination buffer
 * @param  buf_size destination capacity
 * @return number of bytes copied
 */
uint16_t rs485_receive(uint8_t *buf, uint16_t buf_size);

/** @brief  Register (or clear with 0) the per-byte receive hook. */
void rs485_register_rx_byte_hook(rs485_rx_byte_cb_t cb);

/** @brief  Number of bytes currently buffered. */
uint16_t rs485_rx_len(void);

/** @brief  Discard the buffered bytes. */
void rs485_rx_clear(void);

#endif /* BSP_RS485_H */
