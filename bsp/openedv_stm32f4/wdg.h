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

/** @brief  Initialise IWDG with the board-fixed prescaler and reload. */
void iwdg_init(void);

/** @brief  Feed the IWDG. */
void iwdg_feed(void);

/** @brief  Initialise WWDG with the board-fixed counter/window/prescaler (EWI enabled). */
void wwdg_init(void);

/** @brief  Register (or clear) the WWDG early-wakeup callback. */
void wdg_wwdg_register(wdg_wwdg_cb_t cb);

#endif /* BSP_WDG_H */
