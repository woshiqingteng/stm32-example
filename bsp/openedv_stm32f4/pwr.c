/**
 * @file    pwr.c
 * @brief   Power control driver (PVD, WK_UP key and low-power modes).
 *
 * The WK_UP key is handled by the exti driver (single EXTI0 owner); pwr only
 * forwards it to its registered callback. The PVD port layer handles the EXTI
 * flag explicitly instead of using the HAL weak callback. NVIC/GPIO
 * configuration is inlined.
 */

#include "stm32f4xx_hal.h"
#include "pwr.h"
#include "exti.h"

static pwr_pvd_cb_t  g_pvd_cb;
static pwr_wkup_cb_t g_wkup_cb;

static void pwr_wkup_handler(key_id_t id)
{
    (void)id;

    if (g_wkup_cb != NULL)
    {
        g_wkup_cb();
    }
}

static uint32_t pwr_pvd_level_to_hal(pwr_pvd_level_t level)
{
    static const uint32_t levels[8] =
    {
        PWR_PVDLEVEL_0, PWR_PVDLEVEL_1, PWR_PVDLEVEL_2, PWR_PVDLEVEL_3,
        PWR_PVDLEVEL_4, PWR_PVDLEVEL_5, PWR_PVDLEVEL_6, PWR_PVDLEVEL_7
    };

    return levels[(level < 8U) ? (uint32_t)level : 7U];
}

void pwr_wkup_key_init(pwr_wkup_cb_t cb)
{
    g_wkup_cb = cb;

    exti_init();
    exti_register(KEY_WKUP, &pwr_wkup_handler);
}

void pwr_pvd_init(pwr_pvd_level_t level, pwr_pvd_cb_t cb)
{
    PWR_PVDTypeDef pvd = {0};

    g_pvd_cb = cb;

    /* ---- MSP begin: PWR clock ---- */
    __HAL_RCC_PWR_CLK_ENABLE();
    /* ---- MSP end ---- */

    pvd.PVDLevel = pwr_pvd_level_to_hal(level);
    pvd.Mode     = PWR_PVD_MODE_IT_RISING_FALLING;
    HAL_PWR_ConfigPVD(&pvd);

    /* ---- MSP begin: NVIC ---- */
    HAL_NVIC_SetPriority(PVD_IRQn, 3U, 3U);
    HAL_NVIC_EnableIRQ(PVD_IRQn);
    /* ---- MSP end ---- */

    HAL_PWR_EnablePVD();
}

void pwr_enter_sleep(void)
{
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_ResumeTick();
}

void pwr_enter_stop(void)
{
    /* ---- MSP begin: PWR clock ---- */
    __HAL_RCC_PWR_CLK_ENABLE();
    /* ---- MSP end ---- */

    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    HAL_ResumeTick();
}

void pwr_enter_standby(void)
{
    /* ---- MSP begin: PWR clock ---- */
    __HAL_RCC_PWR_CLK_ENABLE();
    /* ---- MSP end ---- */

    /* Reset the backup domain so stale RTC wake-up/alarm state cannot block
     * the WK_UP wake-up (DBP must be set before BDRST). */
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();

    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    HAL_PWR_EnterSTANDBYMode();
}

/* ---- port layer: explicit EXTI flag handling, no HAL weak callback ---- */

void PVD_IRQHandler(void)
{
    if (__HAL_PWR_PVD_EXTI_GET_FLAG() != RESET)
    {
        pwr_pvd_state_t state = (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != RESET) ?
                                PWR_PVD_BELOW : PWR_PVD_ABOVE;

        __HAL_PWR_PVD_EXTI_CLEAR_FLAG();

        if (g_pvd_cb != NULL)
        {
            g_pvd_cb(state);
        }
    }
}
