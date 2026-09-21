/**
 * @file    usart.h
 * @brief   USART1 interface (TX via _write, RX line reception).
 */

#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define USART_REC_LEN 200U

typedef enum
{
    USART_RX_IDLE = 0, /*!< waiting for start */
    USART_RX_CR,       /*!< '\r' seen, waiting for '\n' */
    USART_RX_READY,    /*!< complete line in the buffer */
} usart_rx_state_t;

/** @brief Callback invoked for every byte received on USART1. */
typedef void (*usart_rx_byte_cb_t)(uint8_t byte);

extern UART_HandleTypeDef g_uart1_handle;

/** @brief  Initialise USART1 (PA9 TX / PA10 RX) and start line reception. */
void usart_init(uint32_t baudrate);

/** @brief  Register (or clear with 0) the per-byte receive hook. */
void usart_register_rx_byte_hook(usart_rx_byte_cb_t cb);

usart_rx_state_t usart_rx_state(void);
uint16_t usart_rx_len(void);
const uint8_t *usart_rx_buf(void);
void usart_rx_clear(void);

#endif /* BSP_USART_H */
