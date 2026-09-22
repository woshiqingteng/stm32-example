/**
 * @file    sys.h
 * @brief   System clock interface.
 */

#ifndef BSP_SYS_H
#define BSP_SYS_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  Integer power helper: returns @p m raised to @p n (small exponent). */
static inline uint32_t bsp_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U)
    {
        result *= m;
    }

    return result;
}

/**
 * @brief  Configure HSE + PLL and the AHB/APB prescalers.
 * @param  plln PLL VCO multiplication factor
 * @param  pllm PLL input division factor
 * @param  pllp PLL system clock division factor
 * @param  pllq PLL 48MHz clock division factor
 * @return HAL_OK on success, or the first failing RCC operation status.
 */
HAL_StatusTypeDef sys_clk_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq);

/**
 * @brief  Reconfigure the clocks: switch SYSCLK to HSI, then call sys_clk_init().
 * @param  plln PLL VCO multiplication factor
 * @param  pllm PLL input division factor
 * @param  pllp PLL system clock division factor
 * @param  pllq PLL 48MHz clock division factor
 * @return HAL_OK on success, HAL_TIMEOUT if HSI never becomes ready, or the
 *         first failing RCC operation status.
 */
HAL_StatusTypeDef sys_clk_reconfig(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq);

/** @brief  Get the current HCLK frequency in Hz. */
uint32_t sys_clk_get_hz(void);

#endif /* BSP_SYS_H */
