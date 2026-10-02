/**
 * @file    bsp.h
 * @brief   Board support package entry point.
 */

#ifndef BSP_BSP_H
#define BSP_BSP_H

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"

/* Clock tree (HSE 25 MHz -> PLL): SYSCLK = HCLK = 180 MHz.
 * PCLK1 = HCLK/4 = 45 MHz -> APB1 timer clock = PCLK1 x2 = 90 MHz (TIM2..7,12..14).
 * PCLK2 = HCLK/2 = 90 MHz -> APB2 timer clock = PCLK2 x2 = 180 MHz (TIM1,8..11).
 * Timer update: f = f_TIMxCLK / ((PSC+1)*(ARR+1)). */

/* System clock PLL: HSE 25 MHz -> SYSCLK/HCLK 180 MHz.
 * PLL48CK = VCO/PLLQ = (HSE/PLLM*PLLN)/PLLQ = (25/25*360)/8 = 45 MHz.
 * NOTE: open item - USB OTG FS wants 48 MHz. */
#define BSP_PLLN   360U
#define BSP_PLLM   25U
#define BSP_PLLP   2U
#define BSP_PLLQ   8U

/** @brief  Initialise HAL, system clock, delay, USART1, LEDs and keys. */
void bsp_init(void);

#endif /* BSP_BSP_H */
