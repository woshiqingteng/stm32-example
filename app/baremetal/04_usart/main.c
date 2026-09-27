/**
 * @file    main.c
 * @brief   04_usart: echo USART1 lines received via interrupt.
 *
 * Line assembly (CR+LF terminated) lives here in the application; the usart
 * driver only delivers bytes through the registered callback. A CR not
 * followed by LF discards the whole line; an overlength line is dropped.
 * The prompt/LED cadence is driven from HAL_GetTick() so it is unaffected by
 * the (blocking) printf time.
 */

#include <stdio.h>

#include "bsp.h"
#define USART_POLL_MS          10U
#define USART_PROMPT_PERIOD_MS 2000U
#define USART_BLINK_PERIOD_MS  300U

#define LINE_MAX 199U

static uint8_t           g_line[LINE_MAX + 1U];
static volatile uint16_t g_line_len;
static volatile bool     g_line_ready;
static bool              g_cr_seen;
static bool              g_overflow;

static void line_feed(uint8_t byte)
{
    if (g_line_ready)
    {
        return;
    }

    if (g_cr_seen)
    {
        g_cr_seen = false;
        if (byte == '\n')
        {
            if (!g_overflow)
            {
                g_line_ready = true;
            }
            g_overflow = false;
        }
        else
        {
            g_line_len = 0U;
            g_overflow = false;
        }
        return;
    }

    if (byte == '\r')
    {
        g_cr_seen = true;
        return;
    }

    if (g_overflow)
    {
        return;
    }

    if (g_line_len < LINE_MAX)
    {
        g_line[g_line_len++] = byte;
    }
    else
    {
        g_overflow = true;
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
    printf(APP_BANNER "\r\n");
    usart_set_rx_cb(USART_ID_1, line_feed);

    next_prompt = HAL_GetTick() + USART_PROMPT_PERIOD_MS;
    next_blink  = HAL_GetTick() + USART_BLINK_PERIOD_MS;

    for (;;)
    {
        if (g_line_ready)
        {
            uint16_t len = g_line_len;

            g_line[len] = '\0';
            printf("recv %u bytes: %s\r\n", (unsigned)len, (const char *)g_line);
            g_line_len   = 0U;
            g_line_ready = false;
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
