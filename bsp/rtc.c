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

#define RTC_LSE_RETRY       200U
#define RTC_LSE_POLL_MS     5U

#define RTC_ASYNC_PREDIV    0x7FU
#define RTC_SYNC_PREDIV     0xFFU

#define RTC_INIT_SUCCESS    0U
#define RTC_INIT_FAILURE    1U

#define RTC_WKUP_IRQ_PRIORITY    2U
#define RTC_WKUP_IRQ_SUBPRIORITY 2U

/* Gregorian week calculation constants (valid for 1901..2099). */
#define RTC_CENTURY_BASE       19U
#define RTC_YEARS_PER_CENTURY  100U
#define RTC_LEAP_YEAR_INTERVAL 4U
#define RTC_DAYS_PER_WEEK      7U
#define RTC_MARCH_MONTH        3U
#define RTC_FIRST_MONTH        1U

static RTC_HandleTypeDef g_rtc_handle;
static rtc_wakeup_cb_t   g_rtc_wakeup_cb;

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

void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *ampm)
{
    RTC_TimeTypeDef time = {0};

    (void)HAL_RTC_GetTime(&g_rtc_handle, &time, RTC_FORMAT_BIN);

    *hour = time.Hours;
    *min  = time.Minutes;
    *sec  = time.Seconds;
    *ampm = (uint8_t)time.TimeFormat;
}

HAL_StatusTypeDef rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t ampm)
{
    RTC_TimeTypeDef time = {0};

    time.Hours          = hour;
    time.Minutes        = min;
    time.Seconds        = sec;
    time.TimeFormat     = ampm;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    return HAL_RTC_SetTime(&g_rtc_handle, &time, RTC_FORMAT_BIN);
}

void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week)
{
    RTC_DateTypeDef rtc_date = {0};

    (void)HAL_RTC_GetDate(&g_rtc_handle, &rtc_date, RTC_FORMAT_BIN);

    *year  = rtc_date.Year;
    *month = rtc_date.Month;
    *date  = rtc_date.Date;
    *week  = rtc_date.WeekDay;
}

HAL_StatusTypeDef rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week)
{
    RTC_DateTypeDef rtc_date = {0};

    rtc_date.Year    = year;
    rtc_date.Month   = month;
    rtc_date.Date    = date;
    rtc_date.WeekDay = week;

    return HAL_RTC_SetDate(&g_rtc_handle, &rtc_date, RTC_FORMAT_BIN);
}

uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t temp;
    uint8_t  year_h;
    uint8_t  year_l;

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

static void rtc_clock_config(void)
{
    RCC_OscInitTypeDef       osc  = {0};
    RCC_PeriphCLKInitTypeDef pclk = {0};
    uint16_t                 retry = RTC_LSE_RETRY;

    /* Start LSE and wait for it to become ready. */
    RCC->BDCR |= RCC_BDCR_LSEON;

    while ((retry > 0U) && ((RCC->BDCR & RCC_BDCR_LSERDY) == 0U))
    {
        retry--;
        delay_ms(RTC_LSE_POLL_MS);
    }

    pclk.PeriphClockSelection = RCC_PERIPHCLK_RTC;

    if (retry == 0U)
    {
        RCC->BDCR &= ~RCC_BDCR_LSEON;

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
        osc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
        osc.LSEState       = RCC_LSE_ON;
        osc.PLL.PLLState   = RCC_PLL_NONE;
        (void)HAL_RCC_OscConfig(&osc);

        pclk.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        (void)HAL_RCCEx_PeriphCLKConfig(&pclk);

        rtc_write_bkr(RTC_BKP_FLAG_REG, RTC_BKP_FLAG_LSE);
    }
}

uint8_t rtc_init(void)
{
    g_rtc_handle.Instance          = RTC;
    g_rtc_handle.Init.HourFormat   = RTC_HOURFORMAT_24;
    g_rtc_handle.Init.AsynchPrediv = RTC_ASYNC_PREDIV;
    g_rtc_handle.Init.SynchPrediv  = RTC_SYNC_PREDIV;
    g_rtc_handle.Init.OutPut       = RTC_OUTPUT_DISABLE;
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
        return RTC_INIT_FAILURE;
    }

    return RTC_INIT_SUCCESS;
}

void rtc_set_wakeup(uint8_t wksel, uint16_t cnt)
{
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&g_rtc_handle, RTC_FLAG_WUTF);

    (void)HAL_RTCEx_SetWakeUpTimer_IT(&g_rtc_handle, cnt, wksel);

    /* ---- MSP begin: NVIC ---- */
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, RTC_WKUP_IRQ_PRIORITY, RTC_WKUP_IRQ_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    /* ---- MSP end ---- */
}

void rtc_register_wakeup_hook(rtc_wakeup_cb_t cb)
{
    g_rtc_wakeup_cb = cb;
}

void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&g_rtc_handle);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;

    if (g_rtc_wakeup_cb != 0)
    {
        g_rtc_wakeup_cb();
    }
}
