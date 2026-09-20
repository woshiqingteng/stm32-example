/**
 * @file    bsp.c
 * @brief   Board support package initialisation.
 */

#include "stm32f4xx_hal.h"
#include "bsp/bsp.h"
#include "bsp/sys.h"
#include "bsp/delay.h"
#include "bsp/usart.h"
#include "bsp/led.h"

#define BSP_SYSCLK_MHZ  168U

void bsp_init(void)
{
    HAL_Init();
    sys_clk_init(336U, 25U, 2U, 7U);
    delay_init(BSP_SYSCLK_MHZ);
    usart_init(115200U);
    led_init();
}
