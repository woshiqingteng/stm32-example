/**
 * @file    usart.h
 * @brief   Unified USART driver (USART1/USART2) with a shared read/write API;
 *          poll / interrupt / DMA transports are selected through usart_cfg_t.
 *          Line/protocol parsing is deliberately left to the application.
 */

#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/** @brief USART instance selector (id, not the CMSIS USARTx pointer macro). */
typedef enum
{
    USART_ID_1 = 0, /*!< USART1: PA9 TX / PA10 RX */
    USART_ID_2 = 1, /*!< USART2: PA2 TX / PA3 RX  */
    USART_ID_NUM
} usart_id_t;

/** @brief Transport mode for one direction. */
typedef enum
{
    USART_IO_POLL = 0, /*!< blocking (TX) / direct register reads (RX) */
    USART_IO_IT,       /*!< interrupt driven */
    USART_IO_DMA       /*!< DMA driven (RX is IDLE-drained, no stream IRQ) */
} usart_io_t;

/** @brief USART configuration (all 8N1 frame options are adjustable). */
typedef struct
{
    usart_id_t id;
    uint32_t   baudrate;

    uint32_t   word_length;   /*!< UART_WORDLENGTH_8B / _9B */
    uint32_t   stop_bits;     /*!< UART_STOPBITS_1 / _2 */
    uint32_t   parity;        /*!< UART_PARITY_NONE / _EVEN / _ODD */
    uint32_t   mode;          /*!< UART_MODE_TX_RX / TX / RX */
    uint32_t   hw_flow_ctl;   /*!< UART_HWCONTROL_NONE / RTS / CTS / RTS_CTS */
    uint32_t   oversampling;  /*!< UART_OVERSAMPLING_16 / _8 */

    usart_io_t tx;
    usart_io_t rx;
    uint8_t   *rx_buf;        /*!< external receive buffer (required for IT/DMA) */
    uint16_t   rx_size;

    uint32_t   irq_preempt;   /*!< NVIC preemption priority */
    uint32_t   irq_sub;       /*!< NVIC subpriority */
} usart_cfg_t;

/** @brief 8N1 defaults: no flow control, 16x oversampling, tx=POLL, rx=IT, IRQ (3,3). */
#define USART_CFG_DEFAULT(inst, baud) \
    .id = (inst), .baudrate = (baud), \
    .word_length = UART_WORDLENGTH_8B, .stop_bits = UART_STOPBITS_1, \
    .parity = UART_PARITY_NONE, .mode = UART_MODE_TX_RX, \
    .hw_flow_ctl = UART_HWCONTROL_NONE, .oversampling = UART_OVERSAMPLING_16, \
    .tx = USART_IO_POLL, .rx = USART_IO_IT, \
    .irq_preempt = 3U, .irq_sub = 3U

/** @brief Callback invoked for every byte received (IT/DMA receive modes). */
typedef void (*usart_rx_cb_t)(uint8_t byte);

/** @brief  Initialise one USART according to @p cfg. */
void usart_init(const usart_cfg_t *cfg);

/** @brief  Start a transmit. POLL blocks until done; IT/DMA is asynchronous. */
bool usart_write(usart_id_t id, const uint8_t *data, uint32_t len);

/** @brief  True while an asynchronous (IT/DMA) transmit is in progress. */
bool usart_tx_busy(usart_id_t id);

/**
 * @brief  Receive up to @p len bytes.
 * @param  timeout inter-byte timeout in ms (reset after each byte); 0 = non-blocking.
 * @return number of bytes actually read.
 */
uint32_t usart_read(usart_id_t id, uint8_t *data, uint32_t len, uint32_t timeout);

/** @brief  Register (or clear with 0) the per-byte receive callback. */
void usart_set_rx_cb(usart_id_t id, usart_rx_cb_t cb);

#endif /* BSP_USART_H */
