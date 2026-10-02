/**
 * @file    main.c
 * @brief   18_1_pvd: monitor VDD with the programmable voltage detector.
 */

#include <stdio.h>

#include "bsp.h"
#include "pwr.h"

#define PVD_LEVEL   PWR_PVD_LEVEL_2V9  /*!< 2.9 V threshold */
#define PVD_LOOP_MS 10U

static volatile pwr_pvd_state_t g_pvd_state = PWR_PVD_ABOVE;
static volatile uint8_t         g_pvd_event;

static void pvd_cb(pwr_pvd_state_t state)
{
    g_pvd_state = state;
    g_pvd_event = 1U;
}

int main(void)
{
    uint32_t t = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");

    pwr_pvd_init(PVD_LEVEL, &pvd_cb);
    printf("PVD level 2.9V monitor started\r\n");

    for (;;)
    {
        if (g_pvd_event != 0U)
        {
            g_pvd_event = 0U;
            if (g_pvd_state == PWR_PVD_BELOW)
            {
                led_on(LED1);            /* low voltage: light LED1 (as the stock example) */
                printf("PVD low voltage\r\n");
            }
            else
            {
                led_off(LED1);           /* normal: LED1 off */
                printf("PVD high voltage\r\n");
            }
        }

        if ((++t % 20U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(PVD_LOOP_MS);
    }
}
