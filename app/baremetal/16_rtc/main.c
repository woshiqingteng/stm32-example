/**
 * @file    main.c
 * @brief   16_rtc: print time/date/week once per second.
 *
 * The default calendar is only set on the first run, detected through a backup
 * register that survives a backup-domain power-down.
 */

#include <stdio.h>

#include "bsp.h"
#include "rtc.h"

#define RTC_APP_BKP_REG    1U
#define RTC_APP_BKP_MAGIC  0x52544331U  /* "RTC1" */

#define RTC_DEFAULT_HOUR   12U
#define RTC_DEFAULT_MIN    0U
#define RTC_DEFAULT_SEC    0U
#define RTC_DEFAULT_YEAR   26U
#define RTC_DEFAULT_MONTH  1U
#define RTC_DEFAULT_DATE   1U
#define RTC_PRINT_PERIOD_MS 1000U

static volatile uint32_t g_wakeups;

static void rtc_event_cb(rtc_event_t event, uint32_t info, void *user)
{
    (void)info;
    (void)user;

    if (event == RTC_EVENT_WAKEUP)
    {
        g_wakeups++;
        led_toggle(LED1);
    }
}

int main(void)
{
    rtc_datetime_t default_dt = {
        .year = RTC_DEFAULT_YEAR, .month = RTC_DEFAULT_MONTH, .date = RTC_DEFAULT_DATE,
        .week = 0U, .hour = RTC_DEFAULT_HOUR, .min = RTC_DEFAULT_MIN, .sec = RTC_DEFAULT_SEC
    };
    rtc_config_t cfg = { .wakeup_sec = 1U, .cb = rtc_event_cb };

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (rtc_init(&cfg) != RTC_OK)
    {
        printf("rtc init failed\r\n");
    }

    if (rtc_read_bkr(RTC_APP_BKP_REG) != RTC_APP_BKP_MAGIC)
    {
        (void)rtc_set(&default_dt);
        rtc_write_bkr(RTC_APP_BKP_REG, RTC_APP_BKP_MAGIC);
        printf("rtc: default time/date set\r\n");
    }

    printf("rtc: wakeup timer started (1 Hz)\r\n");

    for (;;)
    {
        rtc_datetime_t dt;

        (void)rtc_get(&dt);

        printf("Time: %02u:%02u:%02u  Date: 20%02u-%02u-%02u  Week: %u  Wake:%lu\r\n",
               dt.hour, dt.min, dt.sec, dt.year, dt.month, dt.date, dt.week,
               (unsigned long)g_wakeups);

        led_toggle(LED0);
        delay_ms(RTC_PRINT_PERIOD_MS);
    }
}
