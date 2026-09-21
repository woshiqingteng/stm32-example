/**
 * @file    main.c
 * @brief   17_rng: print hardware random numbers and a 0-9 range value.
 */

#include <stdio.h>
#include "bsp.h"

int main(void)
{
    bsp_init();

    rng_init();
    while (rng_is_ready() == 0U)
    {
        printf("RNG Error!\r\n");
        delay_ms(200);
        rng_init();
    }
    printf("RNG ready\r\n");

    for (;;)
    {
        printf("Random Num: %lu  Random Num[0-9]: %d\r\n",
               (unsigned long)rng_get_random_num(),
               rng_get_random_range(0, 9));

        led_toggle(LED0);
        delay_ms(500);
    }
}
