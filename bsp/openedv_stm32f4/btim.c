/**
 * @file    btim.c
 * @brief   Basic timer (TIM6) interrupt driver. MSP content is inlined.
 */

#include "stm32f4xx_hal.h"
#include "btim.h"

#define BTIM_NVIC_PRIORITY    1U
#define BTIM_NVIC_SUBPRIORITY 3U

static TIM_HandleTypeDef g_btim_handle;
static btim_cb_t         g_btim_cb;

void btim_timx_int_init(uint16_t arr, uint16_t psc)
{
    /* ---- MSP begin: clock + NVIC ---- */
    __HAL_RCC_TIM6_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, BTIM_NVIC_PRIORITY, BTIM_NVIC_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    /* ---- MSP end ---- */

    g_btim_handle.Instance          = TIM6;
    g_btim_handle.Init.Prescaler    = psc;
    g_btim_handle.Init.CounterMode  = TIM_COUNTERMODE_UP;
    g_btim_handle.Init.Period       = arr;
    HAL_TIM_Base_Init(&g_btim_handle);
    HAL_TIM_Base_Start_IT(&g_btim_handle);
}

void btim_timx_int_register(btim_cb_t cb)
{
    g_btim_cb = cb;
}

void TIM6_DAC_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_btim_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_btim_handle, TIM_FLAG_UPDATE);
        if (g_btim_cb != 0)
        {
            g_btim_cb();
        }
    }
}
