/**
 * @file    exti.h
 * @brief   On-board key external-interrupt interface.
 */

#ifndef BSP_EXTI_H
#define BSP_EXTI_H

#include "key.h"

/** @brief Callback invoked from a key EXTI interrupt. */
typedef void (*exti_cb_t)(key_id_t id);

/** @brief  Configure the key pins as EXTI and enable their interrupts. */
void exti_init(void);

/** @brief  Register (or clear) the callback for one key. */
void exti_register(key_id_t id, exti_cb_t cb);

#endif /* BSP_EXTI_H */
