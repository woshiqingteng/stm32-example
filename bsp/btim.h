/**
 * @file    btim.h
 * @brief   Basic timer (TIM6) interrupt interface.
 */

#ifndef BSP_BTIM_H
#define BSP_BTIM_H

#include <stdint.h>

/** @brief Callback invoked from the basic-timer update interrupt. */
typedef void (*btim_cb_t)(void);

/** @brief  Start TIM6 in update-interrupt mode. @param arr Period-1. @param psc Prescaler-1. */
void btim_timx_int_init(uint16_t arr, uint16_t psc);

/** @brief  Register (or clear) the update callback. */
void btim_timx_int_register(btim_cb_t cb);

#endif /* BSP_BTIM_H */
