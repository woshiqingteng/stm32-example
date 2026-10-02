/**
 * @file    pwr.h
 * @brief   Power control (PVD, WK_UP key and low-power modes) interface.
 */

#ifndef BSP_PWR_H
#define BSP_PWR_H

#include <stdint.h>

/** @brief  PVD comparator state. */
typedef enum
{
    PWR_PVD_ABOVE = 0, /*!< VDD is above the configured level */
    PWR_PVD_BELOW = 1  /*!< VDD is below the configured level */
} pwr_pvd_state_t;

/** @brief  PVD threshold level (2.2 V .. 2.9 V). */
typedef enum
{
    PWR_PVD_LEVEL_2V2 = 0,
    PWR_PVD_LEVEL_2V3,
    PWR_PVD_LEVEL_2V4,
    PWR_PVD_LEVEL_2V5,
    PWR_PVD_LEVEL_2V6,
    PWR_PVD_LEVEL_2V7,
    PWR_PVD_LEVEL_2V8,
    PWR_PVD_LEVEL_2V9
} pwr_pvd_level_t;

/** @brief  Callback invoked from the PVD interrupt. */
typedef void (*pwr_pvd_cb_t)(pwr_pvd_state_t state);

/** @brief  Callback invoked when the WK_UP key (PA0) generates an external interrupt. */
typedef void (*pwr_wkup_cb_t)(void);

/** @brief  Initialise the PVD monitor. @param cb may be NULL. */
void pwr_pvd_init(pwr_pvd_level_t level, pwr_pvd_cb_t cb);

/** @brief  Configure PA0 (WK_UP) as a rising-edge EXTI line. @param cb may be NULL. */
void pwr_wkup_key_init(pwr_wkup_cb_t cb);

/** @brief  Enter sleep mode (WFI). The HAL tick is resumed before returning. */
void pwr_enter_sleep(void);

/** @brief  Enter stop mode (WFI). The HAL tick is resumed before returning;
 *          the caller must re-initialise the clocks. */
void pwr_enter_stop(void);

/** @brief  Enable the WK_UP pin and enter standby mode (wake = system reset). */
void pwr_enter_standby(void);

#endif /* BSP_PWR_H */
