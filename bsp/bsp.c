/**
 * @file    bsp.c
 * @brief   Board support package initialisation.
 */

#include "stm32f4xx_hal.h"
#include "bsp.h"

#define BSP_SYSCLK_MHZ  180U

void bsp_init(void)
{
    HAL_Init();
    sys_clk_init(360U, 25U, 2U, 8U);
    delay_init(BSP_SYSCLK_MHZ);
    usart_init(115200U);
    led_init();
    key_init();
}
