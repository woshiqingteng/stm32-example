/**
 * @file    main.c
 * @brief   18_4_standby: KEY0 enters standby mode, WK_UP wakes (system reset).
 *
 * PWR_CSR.SBF is used to tell a standby wake-up apart from a power-on/reset.
 */

#include <stdio.h>

#include "bsp.h"
#include "pwr.h"

#define STANDBY_ENTRY_DELAY_MS 50U
#define STANDBY_LOOP_MS      10U

static void print_boot_cause(void)
{
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        printf("Boot: returned from standby\r\n");
    }
    else
    {
        printf("Boot: power-on/reset\r\n");
    }

    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
}

int main(void)
{
    uint32_t t = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");

    print_boot_cause();

    /* No wake-pin setup: standby wakes via the WK_UP hardware path (EWUP set in
     * pwr_enter_standby()); pwr_wkup_key_init() is only for sleep/stop (EXTI0). */

    printf("KEY0: enter standby  WKUP: wake\r\n");

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            printf("Entering standby mode...\r\n");
            led_on(LED1);
            delay_ms(STANDBY_ENTRY_DELAY_MS);
            pwr_enter_standby();
        }

        if ((++t % 20U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(STANDBY_LOOP_MS);
    }
}
