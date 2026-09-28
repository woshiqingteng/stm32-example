/**
 * @file    main.c
 * @brief   04_usart: echo USART1 lines received via interrupt.
 *
 * Line assembly (CR+LF terminated) lives here in the application; the usart
 * driver only delivers bytes through the registered callback. A CR not
 * followed by LF discards the whole line; an overlength line restarts the
 * buffer immediately, and the excess is received as a new line.
 * The prompt/LED cadence is driven from HAL_GetTick() so it is unaffected by
 * the (blocking) printf time.
 */

#include <stdio.h>

#include "bsp.h"

#define USART_POLL_MS          10U
#define USART_PROMPT_PERIOD_MS 2000U
#define USART_BLINK_PERIOD_MS  300U

#define LINE_MAX_BYTE 199U

typedef enum
{
    LINE_STATE_IDLE      = 0, /*!< no byte yet for the current line */
    LINE_STATE_RECEIVING = 1, /*!< collecting bytes */
    LINE_STATE_CR        = 2, /*!< CR received; LF completes, any other byte discards */
    LINE_STATE_READY     = 3  /*!< complete line waiting for main */
} line_state_t;

static uint8_t               s_rx_buf[128];
static uint8_t               g_line[LINE_MAX_BYTE + 1U];
static volatile line_state_t g_line_state = LINE_STATE_IDLE;
static volatile uint16_t     g_line_len;

static void line_feed(uint8_t byte)
{
    switch (g_line_state)
    {
    case LINE_STATE_IDLE:
        if (byte == '\r')
        {
            g_line_state = LINE_STATE_CR;
        }
        else
        {
            g_line[g_line_len++] = byte;
            g_line_state = LINE_STATE_RECEIVING;
        }
        break;

    case LINE_STATE_RECEIVING:
        if (byte == '\r')
        {
            g_line_state = LINE_STATE_CR;
        }
        else if (g_line_len < LINE_MAX_BYTE)
        {
            g_line[g_line_len++] = byte;
        }
        else
        {
            g_line_len   = 0U;
            g_line_state = LINE_STATE_IDLE;
        }
        break;

    case LINE_STATE_CR:
        if (byte == '\n')
        {
            g_line_state = LINE_STATE_READY;
        }
        else
        {
            g_line_len   = 0U;
            g_line_state = LINE_STATE_IDLE;
        }
        break;

    case LINE_STATE_READY:
    default:
        break;
    }
}

static bool time_due(uint32_t *next, uint32_t period)
{
    if ((int32_t)(HAL_GetTick() - *next) >= 0)
    {
        *next += period;
        return true;
    }
    return false;
}

int main(void)
{
    uint32_t next_prompt;
    uint32_t next_blink;

    bsp_init();

    {
        usart_cfg_t cfg = { USART_CFG_DEFAULT(USART_ID_1) };

        cfg.rx      = USART_IO_IT;
        cfg.rx_buf  = s_rx_buf;
        cfg.rx_size = sizeof(s_rx_buf);
        usart_init(&cfg);
    }

    printf(APP_BANNER "\r\n");
    usart_set_rx_cb(USART_ID_1, &line_feed);

    next_prompt = HAL_GetTick() + USART_PROMPT_PERIOD_MS;
    next_blink  = HAL_GetTick() + USART_BLINK_PERIOD_MS;

    for (;;)
    {
        if (g_line_state == LINE_STATE_READY)
        {
            uint16_t len = g_line_len;

            g_line[len] = '\0';
            printf("recv %u bytes: %s\r\n", (unsigned)len, (const char *)g_line);
            g_line_len   = 0U;
            g_line_state = LINE_STATE_IDLE;
        }

        if (time_due(&next_prompt, USART_PROMPT_PERIOD_MS))
        {
            printf("please input a line ending with CRLF\r\n");
        }
        if (time_due(&next_blink, USART_BLINK_PERIOD_MS))
        {
            led_toggle(LED0);
        }

        delay_ms(USART_POLL_MS);
    }
}
