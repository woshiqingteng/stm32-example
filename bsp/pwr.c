/**
 * @file    pwr.c
 * @brief   Power control driver (PVD, WK_UP key and low-power modes).
 *
 * NVIC and GPIO configuration is inlined into the init functions.
 */

#include "stm32f4xx_hal.h"
#include "pwr.h"

#define PWR_WKUP_GPIO_PORT      GPIOA
#define PWR_WKUP_GPIO_PIN       GPIO_PIN_0
#define PWR_WKUP_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

#define PWR_WKUP_IRQn           EXTI0_IRQn
#define PWR_WKUP_IRQ_PREEMPT    2U
#define PWR_WKUP_IRQ_SUB        2U

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

void pwr_wkup_key_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* ---- MSP begin: GPIO clock ---- */
    PWR_WKUP_GPIO_CLK_ENABLE();
    /* ---- MSP end ---- */

    gpio.Pin   = PWR_WKUP_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_IT_RISING;
    gpio.Pull  = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PWR_WKUP_GPIO_PORT, &gpio);

    /* ---- MSP begin: NVIC ---- */
    HAL_NVIC_SetPriority(PWR_WKUP_IRQn, PWR_WKUP_IRQ_PREEMPT, PWR_WKUP_IRQ_SUB);
    HAL_NVIC_EnableIRQ(PWR_WKUP_IRQn);
    /* ---- MSP end ---- */
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
    bool low = (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != RESET);

    if (g_pvd_hook != 0)
    {
        g_pvd_hook(low);
    }
}

void EXTI0_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(PWR_WKUP_GPIO_PIN);

    if (g_wkup_hook != 0)
    {
        g_wkup_hook();
    }
}
