/**
 * @file    main.c
 * @brief   18_4_standby: KEY0 enters standby mode, WK_UP wakes (system reset).
 *
 * A backup register is used to tell a standby wake-up (backup domain retained)
 * apart from a power-on reset (backup domain reset).
 */

#include <stdio.h>
#include "bsp.h"

#define STANDBY_BKP_REG    1U
#define STANDBY_BKP_MAGIC  0x5354414EU  /* "STAN" */
#define STANDBY_ENTRY_DELAY_MS 50U
#define LOOP_DELAY_MS      10U

static void print_boot_cause(void)
{
    if (rtc_read_bkr(STANDBY_BKP_REG) == STANDBY_BKP_MAGIC)
    {
        printf("Boot: backup domain retained (returned from standby)\r\n");
    }
    else
    {
        printf("Boot: backup domain reset (power-on reset)\r\n");
    }
}

int main(void)
{
    uint32_t t = 0U;

    bsp_init();

    if (rtc_init() != RTC_OK)
    {
        printf("rtc init failed\r\n");
    }

    pwr_wkup_key_init();
    print_boot_cause();
    rtc_write_bkr(STANDBY_BKP_REG, STANDBY_BKP_MAGIC);

    printf("KEY0: enter standby mode, WK_UP: wake\r\n");

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
        delay_ms(LOOP_DELAY_MS);
    }
}
