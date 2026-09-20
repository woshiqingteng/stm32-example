/**
 * @file    wdg.c
 * @brief   IWDG / WWDG driver. MSP content is inlined into the init functions.
 */

#include "stm32f4xx_hal.h"
#include "wdg.h"

IWDG_HandleTypeDef g_iwdg_handle;
WWDG_HandleTypeDef g_wwdg_handle;

static wdg_wwdg_cb_t g_wwdg_cb;

void iwdg_init(uint32_t prer, uint16_t rlr)
{
    g_iwdg_handle.Instance       = IWDG;
    g_iwdg_handle.Init.Prescaler = prer;
    g_iwdg_handle.Init.Reload    = rlr;
    HAL_IWDG_Init(&g_iwdg_handle);
}

void iwdg_feed(void)
{
    HAL_IWDG_Refresh(&g_iwdg_handle);
}

void wwdg_init(uint8_t tr, uint8_t wr, uint32_t fprer)
{
    /* ---- MSP begin: clock + NVIC ---- */
    __HAL_RCC_WWDG_CLK_ENABLE();
    HAL_NVIC_SetPriority(WWDG_IRQn, 2, 3);
    HAL_NVIC_EnableIRQ(WWDG_IRQn);
    /* ---- MSP end ---- */

    g_wwdg_handle.Instance         = WWDG;
    g_wwdg_handle.Init.Prescaler   = fprer;
    g_wwdg_handle.Init.Window      = wr;
    g_wwdg_handle.Init.Counter     = tr;
    g_wwdg_handle.Init.EWIMode     = WWDG_EWI_ENABLE;
    HAL_WWDG_Init(&g_wwdg_handle);
}

void wdg_wwdg_register(wdg_wwdg_cb_t cb)
{
    g_wwdg_cb = cb;
}

void WWDG_IRQHandler(void)
{
    HAL_WWDG_IRQHandler(&g_wwdg_handle);
}

void HAL_WWDG_EarlyWakeupCallback(WWDG_HandleTypeDef *hwwdg)
{
    (void)hwwdg;

    HAL_WWDG_Refresh(&g_wwdg_handle);
    if (g_wwdg_cb != 0)
    {
        g_wwdg_cb();
    }
}
