/**
 * @file    bsp.c
 * @brief   Board support package initialisation.
 */

#include "stm32f4xx_hal.h"
#include "bsp.h"

#define BSP_SYSCLK_MHZ     180U

#define BSP_PLLN_RAW           360U
#define BSP_PLLM_RAW           25U
#define BSP_PLLP_RAW           2U
#define BSP_PLLQ_RAW           8U   /* TODO: VCO/8 = 45 MHz; sys.c comment claims 48 MHz PLL48CK */

void bsp_init(void)
{
    HAL_Init();
    (void)sys_clk_init(BSP_PLLN_RAW, BSP_PLLM_RAW, BSP_PLLP_RAW, BSP_PLLQ_RAW);
    delay_init(BSP_SYSCLK_MHZ);
    usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1) });
    led_init();
    key_init();
}
