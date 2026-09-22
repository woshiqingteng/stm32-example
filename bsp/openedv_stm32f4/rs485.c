/**
 * @file    rs485.c
 * @brief   RS485 driver on USART2. The transceiver direction is controlled by
 *          the PCF8574 expander bit PCF8574_RS485_RE_IO, as in the vendor
 *          example. MSP content is inlined into rs485_init().
 */

#include "stm32f4xx_hal.h"
#include "rs485.h"
#include "pcf8574.h"
#include "delay.h"
#include "sys.h"

#define RS485_DATA_MASK         0xFFU
#define RS485_IRQ_PREEMP        3U
#define RS485_IRQ_SUB           3U
#define RS485_TX_TIMEOUT_MS     1000U
#define RS485_RX_IDLE_MS        10U

static UART_HandleTypeDef g_rs485_handle;

static rs485_rx_byte_cb_t g_rs485_rx_cb;

static uint8_t  g_rs485_rx_buf[RS485_REC_LEN];
static uint16_t g_rs485_rx_cnt;

void rs485_tx_set(uint8_t en)
{
    pcf8574_write_bit(PCF8574_RS485_RE_IO, en);
}

void rs485_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init = {0};

    pcf8574_init();

    /* ---- MSP begin: USART2 clock + PA2/PA3 + NVIC ---- */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Pin       = RS485_TX_GPIO_PIN | RS485_RX_GPIO_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = RS485_GPIO_AF;
    HAL_GPIO_Init(RS485_TX_GPIO_PORT, &gpio_init);

    HAL_NVIC_SetPriority(RS485_UX_IRQn, RS485_IRQ_PREEMP, RS485_IRQ_SUB);
    HAL_NVIC_EnableIRQ(RS485_UX_IRQn);
    /* ---- MSP end ---- */

    g_rs485_handle.Instance          = RS485_UX;
    g_rs485_handle.Init.BaudRate     = baudrate;
    g_rs485_handle.Init.WordLength   = UART_WORDLENGTH_8B;
    g_rs485_handle.Init.StopBits     = UART_STOPBITS_1;
    g_rs485_handle.Init.Parity       = UART_PARITY_NONE;
    g_rs485_handle.Init.Mode         = UART_MODE_TX_RX;
    g_rs485_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    g_rs485_handle.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&g_rs485_handle);

    g_rs485_rx_cnt = 0U;

    __HAL_UART_ENABLE_IT(&g_rs485_handle, UART_IT_RXNE);

    rs485_tx_set(0U); /* default to receive */
}

void rs485_send(const uint8_t *buf, uint16_t len)
{
    rs485_tx_set(1U);
    (void)HAL_UART_Transmit(&g_rs485_handle, (uint8_t *)buf, len, RS485_TX_TIMEOUT_MS);
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
    g_rs485_rx_cb = cb;
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

void USART2_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&g_rs485_handle, UART_FLAG_RXNE) != RESET)
    {
        uint8_t byte = (uint8_t)(RS485_UX->DR & RS485_DATA_MASK);

        if (g_rs485_rx_cb != 0)
        {
            g_rs485_rx_cb(byte);
        }

        if (g_rs485_rx_cnt < RS485_REC_LEN)
        {
            g_rs485_rx_buf[g_rs485_rx_cnt++] = byte;
        }
    }

    if (__HAL_UART_GET_FLAG(&g_rs485_handle, UART_FLAG_ORE) != RESET)
    {
        __HAL_UART_CLEAR_OREFLAG(&g_rs485_handle);
    }
}
