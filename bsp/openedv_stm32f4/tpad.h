/**
 * @file    tpad.h
 * @brief   Capacitive touch key (TPAD) on TIM2_CH1 / PA5.
 */

#ifndef BSP_TPAD_H
#define BSP_TPAD_H

#include <stdbool.h>
#include <stdint.h>

/** @brief  Touch-key driver status. */
typedef enum
{
    TPAD_OK = 0,
    TPAD_ERROR
} tpad_status_t;

/** @brief  Calibrate the touch key. @param psc Prescaler. */
tpad_status_t tpad_init(uint16_t psc);

/** @brief  Scan the touch key. @param continuous true to allow repeat. @return true if touched. */
bool tpad_scan(bool continuous);

#endif /* BSP_TPAD_H */
