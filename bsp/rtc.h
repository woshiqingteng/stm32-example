/**
 * @file    rtc.h
 * @brief   Real-time clock (RTC) and backup register interface.
 *
 * The RTC is driven from LSE when available, otherwise from LSI. The selected
 * source is recorded in backup register 0 (0x5050 = LSE, 0x5051 = LSI).
 */

#ifndef BSP_RTC_H
#define BSP_RTC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief Callback invoked from the RTC periodic wake-up interrupt. */
typedef void (*rtc_wakeup_cb_t)(void);

/**
 * @brief  Initialise the RTC (LSE with LSI fallback); the calendar is not set.
 * @return 0 on success, 1 if HAL_RTC_Init() failed.
 */
uint8_t rtc_init(void);

/** @brief  Read backup register @p bkrx (0..31). */
uint32_t rtc_read_bkr(uint32_t bkrx);

/** @brief  Write @p data to backup register @p bkrx (0..31). */
void rtc_write_bkr(uint32_t bkrx, uint32_t data);

/** @brief  Get the time. @param ampm 0 = AM/24H, 1 = PM. */
void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *ampm);

/** @brief  Set the time. @param ampm 0 = AM/24H, 1 = PM. */
HAL_StatusTypeDef rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t ampm);

/** @brief  Get the date. @param week Weekday (1..7). */
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week);

/** @brief  Set the date. @param year 0..99. @param week Weekday (1..7). */
HAL_StatusTypeDef rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week);

/** @brief  Start the periodic wake-up interrupt. @param wksel RTC_WAKEUPCLOCK_x. */
void rtc_set_wakeup(uint8_t wksel, uint16_t cnt);

/** @brief  Register (or clear) the wake-up callback. */
void rtc_register_wakeup_hook(rtc_wakeup_cb_t cb);

/** @brief  Weekday (1..7) for a Gregorian date between 1901 and 2099. */
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t day);

#endif /* BSP_RTC_H */
