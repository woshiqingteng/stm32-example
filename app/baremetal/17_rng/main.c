/**
 * @file    main.c
 * @brief   17_rng: print hardware random numbers and a 0-9 range value.
 */

#include <stdio.h>

#include "bsp.h"
#include "rng.h"

#define RNG_RANGE_MIN  0
#define RNG_RANGE_MAX  9
#define RNG_BLINK_MS   500U

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    if (rng_init() != RNG_OK)
    {
        printf("RNG Error!\r\n");
    }

    for (;;)
    {
        uint32_t num;
        int ranged;

        if ((rng_get(&num) == RNG_OK) &&
            (rng_get_range(RNG_RANGE_MIN, RNG_RANGE_MAX, &ranged) == RNG_OK))
        {
            printf("Random Num: %lu  Random Num[0-9]: %d\r\n",
                   (unsigned long)num, ranged);
        }

        led_toggle(LED0);
        delay_ms(RNG_BLINK_MS);
    }
}
