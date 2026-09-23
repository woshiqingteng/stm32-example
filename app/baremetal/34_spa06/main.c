/**
 * @file    main.c
 * @brief   34_spa06: SPA06 barometric pressure / temperature test. The
 *          compensated pressure and temperature are printed over USART1
 *          (fixed-point formatting, one packet per sample).
 */

#include <stdio.h>
#include "bsp.h"

#define SAMPLE_PERIOD   200U

static void format_fixed(char *buf, const char *label, const char *unit, int32_t value_x100)
{
    int32_t mag = (value_x100 < 0) ? -value_x100 : value_x100;

    sprintf(buf, "%s%s%d.%02d%s", label, (value_x100 < 0) ? "-" : "",
            (int)(mag / 100), (int)(mag % 100), unit);
}

int main(void)
{
    char line[48];
    spa06_result_t sample;
    uint8_t t = 0U;

    bsp_init();

    if (spa06_init() != 0U)
    {
        printf("SPA06 check failed\r\n");
    }
    else
    {
        printf("SPA06 ready\r\n");
    }

    printf("34_spa06 ready\r\n");

    for (;;)
    {
        delay_ms(SAMPLE_PERIOD);
        t++;

        if (t >= 5U)
        {
            t = 0U;
            spa06_get_data(&sample);

            format_fixed(line, "P: ", " hPa", (int32_t)(sample.pcomp * 100.0f));
            printf("%s", line);
            printf("  raw=%ld\r\n", (long)sample.praw);

            format_fixed(line, "T: ", " C", (int32_t)(sample.tcomp * 100.0f));
            printf("%s\r\n", line);

            led_toggle(LED0);
        }
    }
}
