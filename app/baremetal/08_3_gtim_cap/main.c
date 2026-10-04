/**
 * @file    main.c
 * @brief   08_3_gtim_cap: TIM5_CH1 (PA0) capture, print the high-level width.
 *
 * The TIM5 driver only reports capture/overflow events through a callback; the
 * measurement state machine (and the unit scaling) lives here in the app.
 * 1 tick = 1 us, 32-bit counter with a software overflow accumulator, so the
 * measurable width is (overflows * 2^32 + CNT) us.
 */

#include <stdio.h>

#include "bsp.h"
#include "tim.h"

#define GTIM_CAP_ARR     0xFFFFFFFFU
#define GTIM_CAP_PSC     90U
/* tick = 90 MHz/90 = 1 MHz -> 1 us */
#define GTIM_CAP_LOOP_MS 200U

/** @brief High-level capture state. */
typedef enum
{
    CAP_IDLE   = 0,   /*!< waiting for a rising edge */
    CAP_RISING = 1,   /*!< high level in progress */
    CAP_READY  = 2,   /*!< width available for main */
} cap_state_t;

static volatile cap_state_t g_cap_state     = CAP_IDLE;
static volatile uint32_t    g_cap_overflows = 0U;
static volatile uint64_t    g_cap_width     = 0U;

/* Capture callback: runs in the TIM5 capture interrupt. */
static tim_edge_t cap_event(tim_cap_ch_t ch, uint32_t value, tim_edge_t edge)
{
    tim_edge_t next = TIM_EDGE_RISING;

    (void)ch;

    switch (g_cap_state)
    {
    case CAP_IDLE:
        if (edge == TIM_EDGE_RISING)
        {
            g_cap_overflows = 0U;
            g_cap_state     = CAP_RISING;
            next            = TIM_EDGE_FALLING;   /* measure the high level */
        }
        break;

    case CAP_RISING:
        if (edge == TIM_EDGE_FALLING)
        {
            g_cap_width = ((uint64_t)g_cap_overflows << 32) + (uint64_t)value;
            g_cap_state = CAP_READY;
            next        = TIM_EDGE_RISING;
        }
        else
        {
            next = TIM_EDGE_FALLING;
        }
        break;

    case CAP_READY:
    default:
        /* Wait for main to consume; stay armed for a rising edge. */
        break;
    }

    return next;
}

/* Update callback: a 32-bit wrap while a high level is in progress. */
static void cap_overflow(void)
{
    if (g_cap_state == CAP_RISING)
    {
        g_cap_overflows++;
    }
}

/* Print a 64-bit value in decimal without relying on %llu (nano.specs). */
static void print_u64(uint64_t value)
{
    char     buf[20];
    uint32_t i = 0U;

    if (value == 0U)
    {
        printf("0");
        return;
    }

    while ((value != 0U) && (i < sizeof(buf)))
    {
        buf[i++] = (char)('0' + (uint32_t)(value % 10U));
        value /= 10U;
    }

    while (i > 0U)
    {
        printf("%c", buf[--i]);
    }
}

/* Print a duration (us) with a human unit: us / ms / s / min / h, 3 decimals. */
static void print_duration(uint64_t us)
{
    static const uint64_t    scale[] = { 1ULL, 1000ULL, 1000000ULL,
                                         60000000ULL, 3600000000ULL };
    static const char *const unit[]  = { "us", "ms", "s", "min", "h" };
    uint32_t i = 0U;

    while ((i < 4U) && (us >= scale[i + 1U]))
    {
        i++;
    }

    if (i == 0U)
    {
        print_u64(us);
        printf(" us\r\n");
    }
    else
    {
        uint64_t remainder = us % scale[i];
        uint32_t frac      = (uint32_t)((remainder * 1000ULL) / scale[i]);

        print_u64(us / scale[i]);
        printf(".%03lu %s\r\n", (unsigned long)frac, unit[i]);
    }
}

int main(void)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    bsp_init();
    printf(APP_BANNER "\r\n");

    cfg.id         = TIM_ID_5;
    cfg.mode       = TIM_MODE_IC;
    cfg.channel    = TIM_CH1;
    cfg.polarity   = TIM_POL_HIGH;
    cfg.pull       = TIM_PULL_DOWN;
    cfg.arr        = GTIM_CAP_ARR;
    cfg.psc        = GTIM_CAP_PSC - 1U;
    cfg.update_cb  = &cap_overflow;
    cfg.capture_cb = &cap_event;
    tim_init(&cfg);

    for (;;)
    {
        if (g_cap_state == CAP_READY)
        {
            uint64_t width;

            sys_intx_disable();
            width       = g_cap_width;
            g_cap_state = CAP_IDLE;
            sys_intx_enable();

            printf("HIGH:");
            print_duration(width);
        }

        led_toggle(LED0);
        delay_ms(GTIM_CAP_LOOP_MS);
    }
}
