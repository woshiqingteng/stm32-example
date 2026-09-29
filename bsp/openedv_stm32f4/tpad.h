/**
 * @file    tpad.h
 * @brief   Capacitive touch key (TPAD) on TIM2_CH1 / PA5.
 */

#ifndef BSP_TPAD_H
#define BSP_TPAD_H

#include <stdint.h>

/** @brief  Touch-key driver status. */
typedef enum
{
    TPAD_OK = 0,
    TPAD_ERROR = 1
} tpad_status_t;

/** @brief  Calibrate the touch key. @param psc Counter divider (>= 1). */
tpad_status_t tpad_init(uint16_t psc);

/** @brief  Calibrated no-touch baseline (raw capture count). */
uint32_t tpad_baseline(void);

/** @brief  Raw measurement: max of n charge-time samples. */
uint32_t tpad_get_maxval(uint8_t n);

#endif /* BSP_TPAD_H */
