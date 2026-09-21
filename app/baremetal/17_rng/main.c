/**
 * @file    main.c
 * @brief   17_rng: print hardware random numbers and a 0-9 range value.
 */

#include <stdio.h>
#include "bsp.h"
#include "rng.h"

#define RNG_RANGE_MIN  0
#define RNG_RANGE_MAX  9
#define RNG_RETRY_MS   200U
#define RNG_BLINK_MS   500U

int main(void)
{
    bsp_init();

    rng_init();
    while (rng_is_ready() != RNG_READY)
    {
        printf("RNG Error!\r\n");
        delay_ms(RNG_RETRY_MS);
        rng_init();
    }
    printf("RNG ready\r\n");

    for (;;)
    {
        printf("Random Num: %lu  Random Num[0-9]: %d\r\n",
               (unsigned long)rng_get_random_num(),
               rng_get_random_range(RNG_RANGE_MIN, RNG_RANGE_MAX));

        led_toggle(LED0);
        delay_ms(RNG_BLINK_MS);
    }
}
