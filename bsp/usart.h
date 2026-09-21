/**
 * @file    usart.h
 * @brief   USART1 interface (TX via _write, RX line reception).
 */

#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>

#define USART_REC_LEN 200U

typedef enum
{
    USART_RX_IDLE = 0, /*!< waiting for start */
    USART_RX_CR,       /*!< '\r' seen, waiting for '\n' */
    USART_RX_READY,    /*!< complete line in the buffer */
} usart_rx_state_t;

/** @brief Callback invoked for every byte received on USART1. */
typedef void (*usart_rx_byte_cb_t)(uint8_t byte);

/** @brief  Initialise USART1 (PA9 TX / PA10 RX) and start line reception. */
void usart_init(uint32_t baudrate);

/** @brief  Register (or clear with 0) the per-byte receive hook. */
void usart_register_rx_byte_hook(usart_rx_byte_cb_t cb);

/** @brief  Initialise USART1 TX DMA (DMA2 Stream7 / channel 4). */
void usart_dma_tx_init(void);

/** @brief  Start a non-blocking USART1 TX DMA transfer. */
void usart_dma_tx(const uint8_t *data, uint16_t len);

/** @brief  1 while a DMA transfer is in progress, 0 otherwise. */
uint8_t usart_dma_tx_busy(void);

/** @brief  Current reception state. */
usart_rx_state_t usart_rx_state(void);

/** @brief  Number of bytes in the received line. */
uint16_t usart_rx_len(void);

/** @brief  Received line buffer. */
const uint8_t *usart_rx_buf(void);

/** @brief  Reset reception (ready for a new line). */
void usart_rx_clear(void);

#endif /* BSP_USART_H */
