/**
 * @file    rs485.c
 * @brief   RS485 half-duplex driver on USART2. The transceiver direction is
 *          controlled by the PCF8574 expander bit PCF8574_RS485_RE_IO, as in
 *          the vendor example. Transport is provided by the unified usart
 *          driver (USART_ID_2, RX interrupt); the per-byte hook fills the local
 *          buffer so rs485_receive() keeps its line-oriented API.
 */

#include "stm32f4xx_hal.h"
#include "rs485.h"
#include "io_expand.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"

#define RS485_RX_IDLE_MS        10U

/* Small landing buffer for the usart driver: its presence enables RX IT. */
static uint8_t g_rs485_it_buf[8];

static uint8_t  g_rs485_rx_buf[RS485_REC_LEN];
static uint16_t g_rs485_rx_cnt;

static void rs485_rx_byte_hook(uint8_t byte);

void rs485_tx_set(uint8_t en)
{
    io_expand_write_bit(PCF8574_RS485_RE_IO, en);
}

void rs485_init(uint32_t baudrate)
{
    usart_cfg_t cfg = { USART_CFG_DEFAULT(USART_ID_2) };

    io_expand_init();

    cfg.baudrate = baudrate;
    cfg.rx       = USART_IO_IT;
    cfg.rx_buf   = g_rs485_it_buf;
    cfg.rx_size  = sizeof(g_rs485_it_buf);
    usart_init(&cfg);

    g_rs485_rx_cnt = 0U;
    usart_set_rx_cb(USART_ID_2, &rs485_rx_byte_hook);

    rs485_tx_set(0U); /* default to receive */
}

void rs485_send(const uint8_t *buf, uint16_t len)
{
    rs485_tx_set(1U);
    (void)usart_write(USART_ID_2, buf, len);
    rs485_tx_set(0U);
}

uint16_t rs485_receive(uint8_t *buf, uint16_t buf_size)
{
    uint16_t i;
    uint16_t len;

    delay_ms(RS485_RX_IDLE_MS); /* wait for the line to go idle */

    sys_intx_disable();
    len = g_rs485_rx_cnt;
    if (len > buf_size)
    {
        len = buf_size;
    }

    for (i = 0U; i < len; i++)
    {
        buf[i] = g_rs485_rx_buf[i];
    }

    g_rs485_rx_cnt = 0U;
    sys_intx_enable();

    return len;
}

void rs485_register_rx_byte_hook(rs485_rx_byte_cb_t cb)
{
    usart_set_rx_cb(USART_ID_2, cb);
}

uint16_t rs485_rx_len(void)
{
    return g_rs485_rx_cnt;
}

void rs485_rx_clear(void)
{
    sys_intx_disable();
    g_rs485_rx_cnt = 0U;
    sys_intx_enable();
}

static void rs485_rx_byte_hook(uint8_t byte)
{
    if (g_rs485_rx_cnt < RS485_REC_LEN)
    {
        g_rs485_rx_buf[g_rs485_rx_cnt++] = byte;
    }
}