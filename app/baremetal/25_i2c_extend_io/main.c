/**
 * @file    main.c
 * @brief   25_i2c_extend_io: PCF8574 8-bit IO expander demo (after the vendor
 *          example). KEY0 toggles the buzzer; the expander INT line is polled
 *          and reading EX_IO clears it (LED1 mirrors the EX_IO input). LED0 is
 *          the run heartbeat. Status is reported over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "io_expand.h"

#define LOOP_MS             10U
#define LED_HEARTBEAT_TICK  20U   /* 20 * 10 ms = 200 ms */

static uint8_t g_beepsta = 1U;   /* buzzer is active low: 1 = silent */
static uint8_t g_tick    = 0U;

/* One demo step: KEY0 toggles the buzzer; an asserted INT reads EX_IO (which
 * clears it) and toggles LED1; LED0 is a 200 ms run heartbeat. */
static void expand_show(void)
{
    if (key_scan(false) == KEY0)
    {
        g_beepsta ^= 1U;
        io_expand_write_bit(IO_EXPAND_BEEP, g_beepsta);
        printf("BEEP %s\r\n", (g_beepsta == 0U) ? "on" : "off");
    }

    if (io_expand_int_asserted())                        /* INT low = asserted */
    {
        uint8_t ex = io_expand_read_bit(IO_EXPAND_EX);   /* reading clears INT */

        if (ex == 0U)
        {
            led_toggle(LED1);
        }
    }

    if (++g_tick >= LED_HEARTBEAT_TICK)
    {
        g_tick = 0U;
        led_toggle(LED0);
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    if (io_expand_init() != 0U)
    {
        printf("PCF8574 check failed\r\n");
    }

    printf("KEY0: BEEP ON/OFF\r\n");

    for (;;)
    {
        expand_show();
        delay_ms(LOOP_MS);
    }
}
