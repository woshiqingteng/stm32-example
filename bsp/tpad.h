/**
 * @file    tpad.h
 * @brief   Capacitive touch key (TPAD) on TIM2_CH1 / PA5.
 */

#ifndef BSP_TPAD_H
#define BSP_TPAD_H

#include <stdbool.h>
#include <stdint.h>

/** @brief  Calibrate the touch key. @param psc Prescaler. @return 0 ok, 1 fail. */
uint8_t tpad_init(uint16_t psc);

/** @brief  Scan the touch key. @param continuous true to allow repeat. @return 1 if touched. */
uint8_t tpad_scan(bool continuous);

#endif /* BSP_TPAD_H */
