/**
 * @file    usmart_port.c
 * @brief   USMART port layer: TIM4 1 us time base and USART1 byte input.
 *
 * The command line is assembled by a hook registered with the USART driver, one
 * byte per interrupt, and handed to usmart_scan() once a CR/LF terminates it.
 * TIM4 free runs with a 1 us tick and a full 16 bit period so that
 * usmart_timx_get_time() can report a function's run-time in microseconds.
 */

#include "stm32f4xx_hal.h"
#include "usmart.h"
#include "usmart_port.h"
#include "usart.h"
#include "tim.h"

#define USMART_RX_BUF_LEN PARM_LEN /*!< command line buffer, must cover PARM_LEN */
#define USMART_HZ_PER_MHZ 1000000U

/* TIM4 time base (board specific, deliberately kept out of the public header). */
#define USMART_TIMX              TIM4

/** @brief Command line reception state. */
typedef enum
{
    USMART_RX_IDLE = 0, /*!< waiting for the first byte */
    USMART_RX_RECEIVING,/*!< line in progress */
    USMART_RX_READY     /*!< complete line waiting to be consumed */
} usmart_rx_state_t;

static uint8_t           g_usmart_rx_buf[USMART_RX_BUF_LEN + 1U];
static uint16_t          g_usmart_rx_len;
static usmart_rx_state_t g_usmart_rx_state;

static void usmart_rx_byte_hook(uint8_t byte)
{
    if ((byte == '\r') || (byte == '\n'))
    {
        if (g_usmart_rx_len != 0U)
        {
            g_usmart_rx_buf[g_usmart_rx_len] = '\0';
            g_usmart_rx_state = USMART_RX_READY;
        }
    }
    else if (g_usmart_rx_state != USMART_RX_READY)
    {
        if (g_usmart_rx_len < USMART_RX_BUF_LEN)
        {
            g_usmart_rx_buf[g_usmart_rx_len++] = byte;
            g_usmart_rx_state = USMART_RX_RECEIVING;
        }
        else
        {
            g_usmart_rx_len   = 0U;
            g_usmart_rx_state = USMART_RX_IDLE;
        }
    }
}

char *usmart_get_input_string(void)
{
    char *pbuf = 0;

    if (g_usmart_rx_state == USMART_RX_READY)
    {
        g_usmart_rx_buf[g_usmart_rx_len] = '\0';
        pbuf              = (char *)g_usmart_rx_buf;
        g_usmart_rx_len   = 0U;
        g_usmart_rx_state = USMART_RX_IDLE;
    }

    return pbuf;
}

static void usmart_timx_init(uint16_t arr, uint16_t psc)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    cfg.id   = TIM_ID_4;
    cfg.mode = TIM_MODE_BASE;
    cfg.arr  = arr;
    cfg.psc  = psc;
    tim_init(&cfg);
}

void usmart_port_init(uint16_t tclk)
{
    usart_set_rx_cb(USART_ID_1, &usmart_rx_byte_hook);

#if USMART_ENTIMX_SCAN == 1
    {
        uint32_t pclk1    = HAL_RCC_GetPCLK1Freq();
        uint32_t tim_clk  = ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) ? pclk1 : (pclk1 * 2U);
        /* PSC = f_TIM4CLK/1 MHz - 1 = 90 MHz/1 MHz - 1 = 89 -> 1 us tick. */
        uint16_t prescaler = (uint16_t)((tim_clk / USMART_TIMX_TICK_HZ) - 1U);

        (void)tclk; /* system clock in MHz; the TIM4 input clock is derived from RCC */
        usmart_timx_init(USMART_TIMX_PERIOD, prescaler);
    }
#else
    (void)tclk;
#endif
}

void usmart_timx_reset_time(void)
{
    tim_set(TIM_ID_4, TIM_CH1, TIM_PARAM_FLAG, TIM_PEND_UPDATE);
    tim_set(TIM_ID_4, TIM_CH1, TIM_PARAM_COUNT, 0U);
    usmart_dev.runtime = 0U;
}

uint32_t usmart_timx_get_time(void)
{
    if ((tim_get(TIM_ID_4, TIM_CH1, TIM_PARAM_FLAG) & TIM_PEND_UPDATE) != 0U)
    {
        usmart_dev.runtime += (USMART_TIMX_PERIOD + 1U);
    }

    usmart_dev.runtime += tim_get(TIM_ID_4, TIM_CH1, TIM_PARAM_COUNT);
    return usmart_dev.runtime;
}
