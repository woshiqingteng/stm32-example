/**
 * @file    rtc.h
 * @brief   Real-time clock (RTC) and backup register interface.
 *
 * The RTC is driven from LSE when available, otherwise from LSI. The selected
 * source is recorded in backup register 0 (0x5050 = LSE, 0x5051 = LSI).
 *
 * @note Event callbacks run in interrupt context; keep them short and only set
 *       flags for the main loop to handle. The alarm event @c info is the
 *       @ref rtc_alarm_id_t value; the wake-up @c info is the running count.
 */

#ifndef BSP_RTC_H
#define BSP_RTC_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Calendar date and time. @c week is 1..7, or 0 to auto-calculate. */
typedef struct
{
    uint8_t year;   /*!< 0..99 (year 2000..2099) */
    uint8_t month;  /*!< 1..12 */
    uint8_t date;   /*!< 1..31 */
    uint8_t week;   /*!< 1..7, 0 = auto-calculate on set */
    uint8_t hour;   /*!< 0..23 (24-hour format) */
    uint8_t min;    /*!< 0..59 */
    uint8_t sec;    /*!< 0..59 */
} rtc_datetime_t;

/** @brief RTC events delivered to the registered callback. */
typedef enum
{
    RTC_EVENT_WAKEUP = 0,   /*!< Periodic wake-up timer elapsed */
    RTC_EVENT_ALARM,        /*!< Alarm A/B matched */
    RTC_EVENT_TAMPER        /*!< Reserved (not implemented) */
} rtc_event_t;

/** @brief Alarm identifier. */
typedef enum
{
    RTC_ALARM_ID_A = 0,
    RTC_ALARM_ID_B
} rtc_alarm_id_t;

/** @brief RTC initialisation status. */
typedef enum
{
    RTC_OK = 0,
    RTC_ERROR = 1
} rtc_status_t;

/* Alarm field masks: a set bit means the field is ignored ("don't care"). */
#define RTC_ALARM_MASK_DATEWEEK (1U << 0)
#define RTC_ALARM_MASK_HOURS    (1U << 1)
#define RTC_ALARM_MASK_MINUTES  (1U << 2)
#define RTC_ALARM_MASK_SECONDS  (1U << 3)
#define RTC_ALARM_MASK_ALL      (RTC_ALARM_MASK_DATEWEEK | RTC_ALARM_MASK_HOURS | \
                                 RTC_ALARM_MASK_MINUTES | RTC_ALARM_MASK_SECONDS)

/** @brief Event callback (interrupt context). */
typedef void (*rtc_event_cb_t)(rtc_event_t event, uint32_t info, void *user);

/** @brief One-shot RTC configuration. */
typedef struct
{
    uint16_t       wakeup_sec;      /*!< 0 = disabled; 1..65535 s periodic wake-up */
    bool           alarm_en;        /*!< Enable an alarm */
    rtc_alarm_id_t alarm_id;        /*!< Alarm A or B */
    rtc_datetime_t alarm_time;      /*!< Uses hour/min/sec and date or week */
    uint8_t        alarm_mask;      /*!< RTC_ALARM_MASK_xxx */
    bool           alarm_use_weekday; /*!< true: match week; false: match date */
    rtc_event_cb_t cb;              /*!< Event callback, may be NULL */
    void          *user;            /*!< Context passed to the callback */
} rtc_config_t;

/**
 * @brief  Initialise the RTC (LSE with LSI fallback); the calendar is not set.
 * @param  cfg Configuration, or NULL to leave all features disabled.
 * @note   Safe to call more than once; only the first call configures.
 */
rtc_status_t rtc_init(const rtc_config_t *cfg);

/** @brief  Read the current date and time. */
rtc_status_t rtc_get(rtc_datetime_t *dt);

/** @brief  Set the date and time. @c week == 0 is auto-calculated. */
rtc_status_t rtc_set(const rtc_datetime_t *dt);

/* ---- backup registers (RTC BKPxR) ---- */
#define RTC_BKR_REG_COUNT 20U        /*!< F4: BKP0R..BKP19R; register 0 is used by the driver */

/** @brief  Read backup register @p reg (0..19). */
uint32_t rtc_read_bkr(uint32_t reg);

/** @brief  Write @p data to backup register @p reg (0..19). */
void rtc_write_bkr(uint32_t reg, uint32_t data);

#endif /* BSP_RTC_H */
