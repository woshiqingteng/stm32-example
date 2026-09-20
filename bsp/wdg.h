/**
 * @file    wdg.h
 * @brief   IWDG / WWDG interface.
 */

#ifndef BSP_WDG_H
#define BSP_WDG_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief Callback invoked from the WWDG early-wakeup interrupt. */
typedef void (*wdg_wwdg_cb_t)(void);

/** @brief  Initialise IWDG. @param prer IWDG_PRESCALER_x. @param rlr Reload. */
void iwdg_init(uint32_t prer, uint16_t rlr);

/** @brief  Feed the IWDG. */
void iwdg_feed(void);

/** @brief  Initialise WWDG with early-wakeup interrupt. */
void wwdg_init(uint8_t tr, uint8_t wr, uint32_t fprer);

/** @brief  Register (or clear) the WWDG early-wakeup callback. */
void wdg_wwdg_register(wdg_wwdg_cb_t cb);

#endif /* BSP_WDG_H */
