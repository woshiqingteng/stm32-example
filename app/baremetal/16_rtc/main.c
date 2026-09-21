/**
 * @file    main.c
 * @brief   16_rtc: print time/date/week once per second.
 *
 * The default calendar is only set on the first run, detected through a backup
 * register that survives a backup-domain power-down.
 */

#include <stdio.h>
#include "bsp.h"

#define RTC_APP_BKP_REG    1U
#define RTC_APP_BKP_MAGIC  0x52544331U  /* "RTC1" */

#define RTC_DEFAULT_HOUR   12U
#define RTC_DEFAULT_MIN    0U
#define RTC_DEFAULT_SEC    0U
#define RTC_DEFAULT_YEAR   24U
#define RTC_DEFAULT_MONTH  1U
#define RTC_DEFAULT_DATE   1U
#define RTC_DEFAULT_WEEK   1U

int main(void)
{
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;

    bsp_init();

    if (rtc_init() != 0U)
    {
        printf("rtc init failed\r\n");
    }

    if (rtc_read_bkr(RTC_APP_BKP_REG) != RTC_APP_BKP_MAGIC)
    {
        (void)rtc_set_time(RTC_DEFAULT_HOUR, RTC_DEFAULT_MIN, RTC_DEFAULT_SEC, RTC_HOURFORMAT12_AM);
        (void)rtc_set_date(RTC_DEFAULT_YEAR, RTC_DEFAULT_MONTH, RTC_DEFAULT_DATE, RTC_DEFAULT_WEEK);
        rtc_write_bkr(RTC_APP_BKP_REG, RTC_APP_BKP_MAGIC);
        printf("rtc: default time/date set\r\n");
    }

    for (;;)
    {
        rtc_get_time(&hour, &min, &sec, &ampm);
        rtc_get_date(&year, &month, &date, &week);

        printf("Time: %02u:%02u:%02u  Date: 20%02u-%02u-%02u  Week: %u\r\n",
               (unsigned)hour, (unsigned)min, (unsigned)sec,
               (unsigned)year, (unsigned)month, (unsigned)date, (unsigned)week);

        led_toggle(LED0);
        delay_ms(1000);
    }
}
