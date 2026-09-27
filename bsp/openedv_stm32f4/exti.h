/**
 * @file    exti.h
 * @brief   On-board key external-interrupt interface.
 */

#ifndef BSP_EXTI_H
#define BSP_EXTI_H

#include "key.h"

/** @brief Per-key interrupt callback (function pointer). */
typedef void (*exti_cb_t)(key_id_t id);

/** @brief  Configure the key pins as EXTI and enable their interrupts. */
void exti_init(void);

/** @brief  Store the callback for one key id; overwrites any previous one. */
void exti_register(key_id_t id, exti_cb_t cb);

/**
 * @brief  Run the debounced key callbacks. The ISR only latches the edge;
 *         call this periodically from the main loop to fire the callbacks.
 */
void exti_poll(void);

#endif /* BSP_EXTI_H */
