/**
 * @file    usart.h
 * @brief   Unified USART driver (USART1/USART2) with a shared read/write API;
 *          poll / interrupt / DMA transports are selected through usart_cfg_t.
 *          Line/protocol parsing is deliberately left to the application.
 *          This interface is HAL-free (own enums); the HAL is used only inside
 *          usart.c for peripheral/stream initialisation.
 */

#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>
#include <stdbool.h>

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

/** @brief Data word length. */
typedef enum
{
    USART_WORD_8B = 0, /*!< 8 data bits */
    USART_WORD_9B      /*!< 9 data bits */
} usart_word_len_t;

/** @brief Stop bits. */
typedef enum
{
    USART_STOP_1 = 0,  /*!< 1 stop bit */
    USART_STOP_2       /*!< 2 stop bits */
} usart_stop_bits_t;

/** @brief Parity. */
typedef enum
{
    USART_PAR_NONE = 0, /*!< no parity */
    USART_PAR_EVEN,     /*!< even parity */
    USART_PAR_ODD       /*!< odd parity */
} usart_parity_t;

/** @brief Direction. */
typedef enum
{
    USART_DIR_TX = 0,  /*!< transmit only */
    USART_DIR_RX,      /*!< receive only */
    USART_DIR_TX_RX    /*!< transmit and receive */
} usart_mode_t;

/** @brief Hardware flow control. */
typedef enum
{
    USART_FLOW_NONE = 0, /*!< no flow control */
    USART_FLOW_RTS,      /*!< RTS */
    USART_FLOW_CTS,      /*!< CTS */
    USART_FLOW_RTS_CTS   /*!< RTS + CTS */
} usart_flow_t;

/** @brief Oversampling. */
typedef enum
{
    USART_OS_16 = 0, /*!< 16x oversampling */
    USART_OS_8       /*!< 8x oversampling */
} usart_oversampling_t;

/** @brief USART configuration (all 8N1 frame options are adjustable). */
typedef struct
{
    usart_id_t id;
    uint32_t   baudrate;

    usart_word_len_t     word_length;   /*!< data word length */
    usart_stop_bits_t    stop_bits;     /*!< stop bits */
    usart_parity_t       parity;        /*!< parity */
    usart_mode_t         mode;          /*!< direction */
    usart_flow_t         hw_flow_ctl;   /*!< hardware flow control */
    usart_oversampling_t oversampling;  /*!< oversampling */

    usart_io_t tx;
    usart_io_t rx;
    uint8_t   *rx_buf;        /*!< external receive buffer (required for IT/DMA) */
    uint16_t   rx_size;

    uint32_t   irq_preempt;   /*!< NVIC preemption priority */
    uint32_t   irq_sub;       /*!< NVIC subpriority */
} usart_cfg_t;

/** @brief 8N1 defaults at 115200: no flow control, 16x oversampling, tx=POLL, rx=POLL, IRQ (3,3).
 *         Callers needing IT/DMA override .tx/.rx (and .rx_buf/.rx_size) afterwards. */
#define USART_CFG_DEFAULT(inst) \
    .id = (inst), .baudrate = 115200U, \
    .word_length = USART_WORD_8B, .stop_bits = USART_STOP_1, \
    .parity = USART_PAR_NONE, .mode = USART_DIR_TX_RX, \
    .hw_flow_ctl = USART_FLOW_NONE, .oversampling = USART_OS_16, \
    .tx = USART_IO_POLL, .rx = USART_IO_POLL, \
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
