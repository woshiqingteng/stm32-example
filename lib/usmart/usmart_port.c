/**
 * @file    usmart_port.c
 * @brief   USMART port layer: TIM4 1 us time base and USART1 byte input.
 *
 * The command line is assembled by a hook registered with the USART driver, one
 * byte per interrupt, and handed to usmart_scan() once a CR/LF terminates it.
 * TIM4 free runs with a 1 us tick and a full 16 bit period so that
 * usmart_timx_get_time() can report a function's run-time in microseconds.
 */

#include "usmart.h"
#include "usmart_port.h"
#include "usart.h"

#define USMART_RX_BUF_LEN PARM_LEN /*!< command line buffer, must cover PARM_LEN */
#define USMART_HZ_PER_MHZ 1000000U

/** @brief Command line reception state. */
typedef enum
{
    USMART_RX_IDLE = 0, /*!< waiting for the first byte */
    USMART_RX_RECEIVING,/*!< line in progress */
    USMART_RX_READY     /*!< complete line waiting to be consumed */
} usmart_rx_state_t;

static TIM_HandleTypeDef g_usmart_timx;
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
    USMART_TIMX_CLK_ENABLE();

    g_usmart_timx.Instance           = USMART_TIMX;
    g_usmart_timx.Init.Prescaler     = psc;
    g_usmart_timx.Init.CounterMode   = TIM_COUNTERMODE_UP;
    g_usmart_timx.Init.Period        = arr;
    g_usmart_timx.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    (void)HAL_TIM_Base_Init(&g_usmart_timx);
    (void)HAL_TIM_Base_Start(&g_usmart_timx);
}

void usmart_port_init(uint16_t tclk)
{
    usart_register_rx_byte_hook(usmart_rx_byte_hook);

#if USMART_ENTIMX_SCAN == 1
    {
        uint32_t pclk1    = HAL_RCC_GetPCLK1Freq();
        uint32_t tim_clk  = ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) ? pclk1 : (pclk1 * 2U);
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
    __HAL_TIM_CLEAR_FLAG(&g_usmart_timx, TIM_FLAG_UPDATE);
    __HAL_TIM_SET_AUTORELOAD(&g_usmart_timx, USMART_TIMX_PERIOD);
    __HAL_TIM_SET_COUNTER(&g_usmart_timx, 0U);
    usmart_dev.runtime = 0U;
}

uint32_t usmart_timx_get_time(void)
{
    if (__HAL_TIM_GET_FLAG(&g_usmart_timx, TIM_FLAG_UPDATE) == SET)
    {
        usmart_dev.runtime += (USMART_TIMX_PERIOD + 1U);
    }

    usmart_dev.runtime += __HAL_TIM_GET_COUNTER(&g_usmart_timx);
    return usmart_dev.runtime;
}
