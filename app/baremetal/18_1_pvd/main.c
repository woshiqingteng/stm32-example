/**
 * @file    main.c
 * @brief   18_1_pvd: monitor VDD with the programmable voltage detector.
 */

#include <stdio.h>
#include "bsp.h"

#define PVD_LEVEL  PWR_PVDLEVEL_7  /* 2.9 V */
#define LOOP_DELAY_MS 10U

static void pvd_hook(pwr_pvd_state_t state)
{
    if (state == PWR_PVD_BELOW)
    {
        printf("PVD low voltage\r\n");
    }
    else
    {
        printf("PVD high voltage\r\n");
    }
}

int main(void)
{
    uint32_t t = 0U;

    bsp_init();

    pwr_register_pvd_hook(pvd_hook);
    pwr_pvd_init(PVD_LEVEL);
    printf("PVD level 7 (2.9V) monitor started\r\n");

    for (;;)
    {
        if ((++t % 20U) == 0U)
        {
            led_toggle(LED0);
        }
        delay_ms(LOOP_DELAY_MS);
    }
}
