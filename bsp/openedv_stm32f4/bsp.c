/**
 * @file    bsp.c
 * @brief   Board support package initialisation.
 */

#include "stm32f4xx_hal.h"
#include "bsp.h"

#define BSP_SYSCLK_MHZ     180U

#define BSP_PLLN           360U
#define BSP_PLLM           25U
#define BSP_PLLP           2U
#define BSP_PLLQ           8U

#define BSP_USART_BAUDRATE 115200U

void bsp_init(void)
{
    HAL_Init();
    (void)sys_clk_init(BSP_PLLN, BSP_PLLM, BSP_PLLP, BSP_PLLQ);
    delay_init(BSP_SYSCLK_MHZ);
    usart_init(BSP_USART_BAUDRATE);
    led_init();
    key_init();
}
