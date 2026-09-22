/**
 * @file    pwr.c
 * @brief   Power control driver (PVD, WK_UP key and low-power modes).
 *
 * The WK_UP key is handled by the exti driver (single EXTI0 owner); pwr only
 * forwards it to its registered hook. NVIC/GPIO configuration is inlined.
 */

#include "stm32f4xx_hal.h"
#include "pwr.h"
#include "exti.h"

#define PWR_PVD_IRQ_PREEMPT     3U
#define PWR_PVD_IRQ_SUB         3U

static pwr_pvd_hook_t  g_pvd_hook;
static pwr_wkup_hook_t g_wkup_hook;

void pwr_register_pvd_hook(pwr_pvd_hook_t hook)
{
    g_pvd_hook = hook;
}

void pwr_register_wkup_hook(pwr_wkup_hook_t hook)
{
    g_wkup_hook = hook;
}

static void pwr_wkup_handler(key_id_t id)
{
    (void)id;

    if (g_wkup_hook != 0)
    {
        g_wkup_hook();
    }
}

void pwr_wkup_key_init(void)
{
    exti_init();
    exti_register(KEY_WKUP, pwr_wkup_handler);
}

void pwr_pvd_init(uint32_t level)
{
    PWR_PVDTypeDef pvd = {0};

    /* ---- MSP begin: PWR clock ---- */
    __HAL_RCC_PWR_CLK_ENABLE();
    /* ---- MSP end ---- */

    pvd.PVDLevel = level;
    pvd.Mode     = PWR_PVD_MODE_IT_RISING_FALLING;
    HAL_PWR_ConfigPVD(&pvd);

    /* ---- MSP begin: NVIC ---- */
    HAL_NVIC_SetPriority(PVD_IRQn, PWR_PVD_IRQ_PREEMPT, PWR_PVD_IRQ_SUB);
    HAL_NVIC_EnableIRQ(PVD_IRQn);
    /* ---- MSP end ---- */

    HAL_PWR_EnablePVD();
}

void pwr_enter_sleep(void)
{
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

void pwr_enter_stop(void)
{
    /* ---- MSP begin: PWR clock ---- */
    __HAL_RCC_PWR_CLK_ENABLE();
    /* ---- MSP end ---- */

    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
}

void pwr_enter_standby(void)
{
    /* ---- MSP begin: PWR clock ---- */
    __HAL_RCC_PWR_CLK_ENABLE();
    /* ---- MSP end ---- */

    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    HAL_PWR_EnterSTANDBYMode();
}

void PVD_IRQHandler(void)
{
    HAL_PWR_PVD_IRQHandler();
}

void HAL_PWR_PVDCallback(void)
{
    pwr_pvd_state_t state = (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != RESET) ?
                            PWR_PVD_BELOW : PWR_PVD_ABOVE;

    if (g_pvd_hook != 0)
    {
        g_pvd_hook(state);
    }
}
