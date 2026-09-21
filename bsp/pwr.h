/**
 * @file    pwr.h
 * @brief   Power control (PVD, wake-up key and low-power modes) interface.
 */

#ifndef BSP_PWR_H
#define BSP_PWR_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief Callback invoked from the PVD interrupt. @param low true if VDD < level. */
typedef void (*pwr_pvd_hook_t)(bool low);

/** @brief Callback invoked when the WK_UP key (PA0) generates an external interrupt. */
typedef void (*pwr_wkup_hook_t)(void);

/** @brief  Register (or clear) the PVD callback. */
void pwr_register_pvd_hook(pwr_pvd_hook_t hook);

/** @brief  Register (or clear) the WK_UP callback. */
void pwr_register_wkup_hook(pwr_wkup_hook_t hook);

/** @brief  Initialise the PVD monitor. @param level PWR_PVDLEVEL_x. */
void pwr_pvd_init(uint32_t level);

/** @brief  Configure PA0 (WK_UP) as a rising-edge EXTI line. */
void pwr_wkup_key_init(void);

/** @brief  Enter sleep mode (WFI). Caller must call HAL_ResumeTick() on wake. */
void pwr_enter_sleep(void);

/** @brief  Enter stop mode (WFI). Caller must re-init clocks and call HAL_ResumeTick(). */
void pwr_enter_stop(void);

/** @brief  Enable the WK_UP pin and enter standby mode (wake = system reset). */
void pwr_enter_standby(void);

#endif /* BSP_PWR_H */
