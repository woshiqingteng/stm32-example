/**
 * @file    rtc.c
 * @brief   Real-time clock driver. MSP content is inlined into the init functions.
 */

#include "stm32f4xx_hal.h"
#include "delay.h"
#include "rtc.h"

#define RTC_BKP_FLAG_REG    0U
#define RTC_BKP_FLAG_LSE    0x5050U
#define RTC_BKP_FLAG_LSI    0x5051U

#define RTC_LSE_RETRY_COUNT       200U
#define RTC_LSE_POLL_MS     5U

/* f = f_RTC_CLK/((ASYNC+1)(SYNC+1)); LSE: 32768/(128*256) = 1 Hz. */
#define RTC_ASYNC_PREDIV    0x7FU
#define RTC_SYNC_PREDIV     0xFFU

/* Gregorian week calculation constants (valid for 1901..2099). */
#define RTC_CENTURY_BASE       19U
#define RTC_YEARS_PER_CENTURY  100U
#define RTC_LEAP_YEAR_INTERVAL 4U
#define RTC_DAYS_PER_WEEK      7U
#define RTC_MARCH_MONTH        3U
#define RTC_FIRST_MONTH        1U
#define RTC_LAST_MONTH         12U

#define RTC_YEAR_BASE          2000U

static RTC_HandleTypeDef g_rtc_handle;
static rtc_event_cb_t    g_event_cb;
static void             *g_event_user;
static volatile uint32_t g_wakeup_n;
static uint8_t           g_inited;

static const uint8_t g_week_table[12] =
{
    0U, 3U, 3U, 6U, 1U, 4U, 6U, 2U, 5U, 0U, 3U, 5U
};

uint32_t rtc_read_bkr(uint32_t bkrx)
{
    return HAL_RTCEx_BKUPRead(&g_rtc_handle, bkrx);
}

void rtc_write_bkr(uint32_t bkrx, uint32_t data)
{
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&g_rtc_handle, bkrx, data);
}

rtc_status_t rtc_get(rtc_datetime_t *dt)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    if (dt == NULL)
    {
        return RTC_ERROR;
    }

    /* HAL requires GetTime before GetDate. */
    if ((HAL_RTC_GetTime(&g_rtc_handle, &time, RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_GetDate(&g_rtc_handle, &date, RTC_FORMAT_BIN) != HAL_OK))
    {
        *dt = (rtc_datetime_t){0};
        return RTC_ERROR;
    }

    dt->year  = date.Year;
    dt->month = date.Month;
    dt->date  = date.Date;
    dt->week  = date.WeekDay;
    dt->hour  = time.Hours;
    dt->min   = time.Minutes;
    dt->sec   = time.Seconds;

    return RTC_OK;
}

/* Weekday (1..7) for a Gregorian date between 1901 and 2099. */
static uint8_t rtc_calc_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t temp;
    uint8_t  year_h;
    uint8_t  year_l;

    if ((month < RTC_FIRST_MONTH) || (month > RTC_LAST_MONTH))
    {
        return 0U;
    }

    /*
     * Algorithm (valid 1901..2099):
     *   week = (yy + yy/4 + day + month_table[m] - leap_adjust) mod 7, 1..7
     * where yy is the year within the century (shifted by 100 for years 2000+),
     * yy/4 counts leap days, and month_table[] holds the weekday offset of the
     * first day of each month. For a leap year, January and February predate
     * 29 Feb, so one day is subtracted before the final modulo.
     */
    year_h = (uint8_t)(year / RTC_YEARS_PER_CENTURY);
    year_l = (uint8_t)(year % RTC_YEARS_PER_CENTURY);

    if (year_h > RTC_CENTURY_BASE)
    {
        year_l = (uint8_t)(year_l + RTC_YEARS_PER_CENTURY);
    }

    temp = (uint16_t)(year_l + (year_l / RTC_LEAP_YEAR_INTERVAL));
    temp = (uint16_t)(temp % RTC_DAYS_PER_WEEK);
    temp = (uint16_t)(temp + day + g_week_table[month - RTC_FIRST_MONTH]);

    if (((year_l % RTC_LEAP_YEAR_INTERVAL) == 0U) && (month < RTC_MARCH_MONTH))
    {
        temp--;
    }

    temp %= RTC_DAYS_PER_WEEK;

    if (temp == 0U)
    {
        temp = RTC_DAYS_PER_WEEK;
    }

    return (uint8_t)temp;
}

rtc_status_t rtc_set(const rtc_datetime_t *dt)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};
    uint8_t         week;

    if (dt == NULL)
    {
        return RTC_ERROR;
    }

    if ((dt->year > 99U) || (dt->month < 1U) || (dt->month > 12U) ||
        (dt->date < 1U) || (dt->date > 31U) || (dt->hour > 23U) ||
        (dt->min > 59U) || (dt->sec > 59U) || (dt->week > 7U))
    {
        return RTC_ERROR;
    }

    week = dt->week;
    if (week == 0U)
    {
        week = rtc_calc_week((uint16_t)(RTC_YEAR_BASE + dt->year), dt->month, dt->date);
    }

    time.Hours          = dt->hour;
    time.Minutes        = dt->min;
    time.Seconds        = dt->sec;
    time.TimeFormat     = RTC_HOURFORMAT_24;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    date.Year    = dt->year;
    date.Month   = dt->month;
    date.Date    = dt->date;
    date.WeekDay = week;

    if (HAL_RTC_SetTime(&g_rtc_handle, &time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return RTC_ERROR;
    }

    if (HAL_RTC_SetDate(&g_rtc_handle, &date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return RTC_ERROR;
    }

    return RTC_OK;
}

static void rtc_clock_config(void)
{
    RCC_OscInitTypeDef       osc  = {0};
    RCC_PeriphCLKInitTypeDef pclk = {0};
    uint16_t                 retry = RTC_LSE_RETRY_COUNT;

    /* Start LSE and probe for it to decide the RTC clock source. */
    __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);

    while ((retry > 0U) && (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET))
    {
        retry--;
        delay_ms(RTC_LSE_POLL_MS);
    }

    pclk.PeriphClockSelection = RCC_PERIPHCLK_RTC;

    if (retry == 0U)
    {
        __HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);

        osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
        osc.LSIState       = RCC_LSI_ON;
        osc.PLL.PLLState   = RCC_PLL_NONE;
        (void)HAL_RCC_OscConfig(&osc);

        pclk.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
        (void)HAL_RCCEx_PeriphCLKConfig(&pclk);

        rtc_write_bkr(RTC_BKP_FLAG_REG, RTC_BKP_FLAG_LSI);
    }
    else
    {
        /* LSE is already enabled and ready above, so no HAL_RCC_OscConfig
         * (which would repeat the same LSE configuration) is required. */
        pclk.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        (void)HAL_RCCEx_PeriphCLKConfig(&pclk);

        rtc_write_bkr(RTC_BKP_FLAG_REG, RTC_BKP_FLAG_LSE);
    }
}

static void rtc_event_dispatch(rtc_event_t event, uint32_t info)
{
    if (g_event_cb != NULL)
    {
        g_event_cb(event, info, g_event_user);
    }
}

static uint32_t rtc_alarm_mask_to_hal(uint8_t mask)
{
    uint32_t hal_mask = 0U;

    if ((mask & RTC_ALARM_MASK_DATEWEEK) != 0U)
    {
        hal_mask |= RTC_ALARMMASK_DATEWEEKDAY;
    }
    if ((mask & RTC_ALARM_MASK_HOURS) != 0U)
    {
        hal_mask |= RTC_ALARMMASK_HOURS;
    }
    if ((mask & RTC_ALARM_MASK_MINUTES) != 0U)
    {
        hal_mask |= RTC_ALARMMASK_MINUTES;
    }
    if ((mask & RTC_ALARM_MASK_SECONDS) != 0U)
    {
        hal_mask |= RTC_ALARMMASK_SECONDS;
    }

    return hal_mask;
}

rtc_status_t rtc_init(const rtc_config_t *cfg)
{
    if (g_inited != 0U)
    {
        return RTC_OK;
    }

    g_rtc_handle.Instance            = RTC;
    g_rtc_handle.Init.HourFormat     = RTC_HOURFORMAT_24;
    g_rtc_handle.Init.AsynchPrediv   = RTC_ASYNC_PREDIV;
    g_rtc_handle.Init.SynchPrediv    = RTC_SYNC_PREDIV;
    g_rtc_handle.Init.OutPut         = RTC_OUTPUT_DISABLE;
    g_rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    g_rtc_handle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    /* ---- MSP begin: PWR/RTC clocks, backup access, RTC clock source ---- */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_RTC_ENABLE();
    rtc_clock_config();
    /* ---- MSP end ---- */

    if (HAL_RTC_Init(&g_rtc_handle) != HAL_OK)
    {
        return RTC_ERROR;
    }

    g_event_cb   = NULL;
    g_event_user = NULL;
    g_wakeup_n   = 0U;

    if (cfg != NULL)
    {
        g_event_cb   = cfg->cb;
        g_event_user = cfg->user;

        if (cfg->wakeup_sec != 0U)
        {
            /* CK_SPRE is 1 Hz; the counter counts sec-1 periods. */
            (void)HAL_RTCEx_DeactivateWakeUpTimer(&g_rtc_handle);
            __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&g_rtc_handle, RTC_FLAG_WUTF);
            (void)HAL_RTCEx_SetWakeUpTimer_IT(&g_rtc_handle, (uint16_t)(cfg->wakeup_sec - 1U),
                                              RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

            HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 2U, 2U);
            HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
        }

        if (cfg->alarm_en)
        {
            RTC_AlarmTypeDef alarm = {0};
            uint32_t         alarm_id;

            alarm_id = (cfg->alarm_id == RTC_ALARM_ID_A) ? RTC_ALARM_A : RTC_ALARM_B;

            alarm.AlarmTime.Hours          = cfg->alarm_time.hour;
            alarm.AlarmTime.Minutes        = cfg->alarm_time.min;
            alarm.AlarmTime.Seconds        = cfg->alarm_time.sec;
            alarm.AlarmTime.TimeFormat     = RTC_HOURFORMAT_24;
            alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
            alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
            alarm.AlarmSubSecondMask       = RTC_ALARMSUBSECONDMASK_ALL;
            alarm.AlarmMask                = rtc_alarm_mask_to_hal(cfg->alarm_mask);
            alarm.AlarmDateWeekDaySel      = cfg->alarm_use_weekday ? RTC_ALARMDATEWEEKDAYSEL_WEEKDAY
                                                                    : RTC_ALARMDATEWEEKDAYSEL_DATE;
            alarm.AlarmDateWeekDay         = cfg->alarm_use_weekday ? cfg->alarm_time.week
                                                                    : cfg->alarm_time.date;

            (void)HAL_RTC_DeactivateAlarm(&g_rtc_handle, alarm_id);
            (void)HAL_RTC_SetAlarm_IT(&g_rtc_handle, &alarm, RTC_FORMAT_BIN);

            HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 2U, 2U);
            HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
        }
    }

    g_inited = 1U;

    return RTC_OK;
}

/* ---- port layer: the only place coupled to the RTC/EXTI registers ---- */

void RTC_WKUP_IRQHandler(void)
{
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();

    if (__HAL_RTC_WAKEUPTIMER_GET_FLAG(&g_rtc_handle, RTC_FLAG_WUTF) != 0U)
    {
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&g_rtc_handle, RTC_FLAG_WUTF);
        g_wakeup_n++;
        rtc_event_dispatch(RTC_EVENT_WAKEUP, g_wakeup_n);
    }
}

void RTC_Alarm_IRQHandler(void)
{
    __HAL_RTC_ALARM_EXTI_CLEAR_FLAG();

    if ((__HAL_RTC_ALARM_GET_IT_SOURCE(&g_rtc_handle, RTC_IT_ALRA) != 0U) &&
        (__HAL_RTC_ALARM_GET_FLAG(&g_rtc_handle, RTC_FLAG_ALRAF) != 0U))
    {
        __HAL_RTC_ALARM_CLEAR_FLAG(&g_rtc_handle, RTC_FLAG_ALRAF);
        rtc_event_dispatch(RTC_EVENT_ALARM, (uint32_t)RTC_ALARM_ID_A);
    }

    if ((__HAL_RTC_ALARM_GET_IT_SOURCE(&g_rtc_handle, RTC_IT_ALRB) != 0U) &&
        (__HAL_RTC_ALARM_GET_FLAG(&g_rtc_handle, RTC_FLAG_ALRBF) != 0U))
    {
        __HAL_RTC_ALARM_CLEAR_FLAG(&g_rtc_handle, RTC_FLAG_ALRBF);
        rtc_event_dispatch(RTC_EVENT_ALARM, (uint32_t)RTC_ALARM_ID_B);
    }
}

void TAMP_STAMP_IRQHandler(void)
{
    __HAL_RTC_TAMPER_TIMESTAMP_EXTI_CLEAR_FLAG();

    if (__HAL_RTC_TAMPER_GET_FLAG(&g_rtc_handle, RTC_FLAG_TAMP1F) != 0U)
    {
        __HAL_RTC_TAMPER_CLEAR_FLAG(&g_rtc_handle, RTC_FLAG_TAMP1F);
        rtc_event_dispatch(RTC_EVENT_TAMPER, 0U);
    }
}
