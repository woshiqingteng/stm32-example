/**
 * @file    delay.c
 * @brief   SysTick polling delay. The SysTick 1ms interrupt (HAL_IncTick) stays enabled.
 */

#include "stm32f4xx_hal.h"
#include "delay.h"

static uint32_t g_fac_us = 0;

void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk;
}

void delay_us(uint32_t nus)
{
    uint32_t reload = SysTick->LOAD;
    uint32_t ticks  = nus * g_fac_us;
    uint32_t told   = SysTick->VAL;
    uint32_t tnow;
    uint32_t tcnt   = 0;

    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

void delay_ms(uint16_t nms)
{
    delay_us((uint32_t)nms * 1000U);
}

void HAL_Delay(uint32_t Delay)
{
    delay_ms((uint16_t)Delay);
}
