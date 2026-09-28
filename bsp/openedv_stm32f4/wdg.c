/**
 * @file    wdg.c
 * @brief   IWDG / WWDG driver. MSP content is inlined into the init functions.
 */

#include "stm32f4xx_hal.h"
#include "wdg.h"

#define WWDG_IRQ_PRIORITY    2U
#define WWDG_IRQ_SUBPRIORITY 3U

/* Board-fixed watchdog configuration. */
#define IWDG_PRESCALER      IWDG_PRESCALER_64  /*!< LSI ~32 kHz input */
#define IWDG_RELOAD    500U               /*!< ~1.0 s timeout (500 * 64 / 32 kHz) */
#define WWDG_PRESCALER      WWDG_PRESCALER_8   /*!< PCLK1/8 input */
#define WWDG_COUNTER   0x7FU              /*!< T[6:0] counter reload */
#define WWDG_WINDOW    0x5FU              /*!< W[5:0] refresh window */

IWDG_HandleTypeDef g_iwdg_handle;
WWDG_HandleTypeDef g_wwdg_handle;

static wdg_wwdg_cb_t g_wwdg_cb;

void iwdg_init(void)
{
    g_iwdg_handle.Instance       = IWDG;
    g_iwdg_handle.Init.Prescaler = IWDG_PRESCALER;
    g_iwdg_handle.Init.Reload    = IWDG_RELOAD;
    HAL_IWDG_Init(&g_iwdg_handle);
}

void iwdg_feed(void)
{
    HAL_IWDG_Refresh(&g_iwdg_handle);
}

void wwdg_init(void)
{
    /* ---- MSP begin: clock + NVIC ---- */
    __HAL_RCC_WWDG_CLK_ENABLE();
    HAL_NVIC_SetPriority(WWDG_IRQn, WWDG_IRQ_PRIORITY, WWDG_IRQ_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(WWDG_IRQn);
    /* ---- MSP end ---- */

    g_wwdg_handle.Instance         = WWDG;
    g_wwdg_handle.Init.Prescaler   = WWDG_PRESCALER;
    g_wwdg_handle.Init.Window      = WWDG_WINDOW;
    g_wwdg_handle.Init.Counter     = WWDG_COUNTER;
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
