/**
 * @file    sys.h
 * @brief   System clock interface.
 */

#ifndef BSP_SYS_H
#define BSP_SYS_H

#include <stdint.h>

/**
 * @brief  Configure HSE + PLL and the AHB/APB prescalers.
 * @param  plln PLL VCO multiplication factor
 * @param  pllm PLL input division factor
 * @param  pllp PLL system clock division factor
 * @param  pllq PLL 48MHz clock division factor
 */
void sys_clk_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq);

/**
 * @brief  Reconfigure the clocks: switch SYSCLK to HSI, then call sys_clk_init().
 * @param  plln PLL VCO multiplication factor
 * @param  pllm PLL input division factor
 * @param  pllp PLL system clock division factor
 * @param  pllq PLL 48MHz clock division factor
 */
void sys_clk_reconfig(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq);

/** @brief  Get the current HCLK frequency in Hz. */
uint32_t sys_clk_get_hz(void);

#endif /* BSP_SYS_H */
