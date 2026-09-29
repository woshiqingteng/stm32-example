/**
 * @file    main.c
 * @brief   10_tpad: capacitive touch key toggles LED1.
 */

#include <stdbool.h>
#include <stdio.h>

#include "bsp.h"
#include "tpad.h"

#define TPAD_LOOP_MS 10U
#define TPAD_GATE_VAL 50U
#define TPAD_SAMPLE   3U
#define TPAD_LOCK     3U

/* Touch decision state machine (lives in the app). */
typedef enum
{
    TPAD_IDLE = 0,
    TPAD_PRESSED
} tpad_state_t;

static tpad_state_t g_touch = TPAD_IDLE;
static uint8_t      g_release;

/* Report a touch once per press; release after TPAD_LOCK scans. */
static bool tpad_touched(void)
{
    uint32_t v = tpad_get_maxval(TPAD_SAMPLE);
    bool touched = false;

    if (v > (uint32_t)tpad_baseline() + TPAD_GATE_VAL)
    {
        if (g_touch == TPAD_IDLE)
        {
            touched = true;
        }
        g_touch   = TPAD_PRESSED;
        g_release = TPAD_LOCK;
    }

    if ((g_release != 0U) && (--g_release == 0U))
    {
        g_touch = TPAD_IDLE;
    }

    return touched;
}

int main(void)
{
    uint32_t blink = 0;

    bsp_init();
    printf(APP_BANNER "\r\n");

    /* tpad_init(2): counter divider = 2 -> charge time = count * 2 / 90 MHz */
    if (tpad_init(2) != TPAD_OK)
    {
        printf("tpad init failed\r\n");
    }

    for (;;)
    {
        if (tpad_touched())
        {
            led_toggle(LED1);
        }

        if ((++blink % 15U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(TPAD_LOOP_MS);
    }
}
