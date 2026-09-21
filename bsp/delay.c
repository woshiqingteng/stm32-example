/**
 * @file    delay.c
 * @brief   SysTick based delay, following the vendor delay.c structure.
 *
 * SysTick_Handler() is provided here for both builds:
 *   baremetal : only keeps the HAL tick alive (HAL_IncTick).
 *   freertos  : also chains to the RTOS tick once the scheduler is running.
 * The OS branch uses the FreeRTOS API directly (vTaskSuspendAll/xTaskResumeAll,
 * vTaskDelay, xPortIsInsideInterrupt, xTaskGetSchedulerState).
 */

#include "stm32f4xx_hal.h"
#include "delay.h"

#define US_PER_MS   1000U
#define MS_PER_SEC  1000U

static uint32_t g_fac_us = 0;

#if USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

/* Provided by portable/GCC/ARM_CM4F/port.c (not declared in the public headers). */
void xPortSysTickHandler(void);

static uint16_t g_fac_ms = 0;
#endif

void SysTick_Handler(void)
{
    HAL_IncTick();

#if USE_FREERTOS
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
#endif
}

void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk;

#if USE_FREERTOS
    g_fac_ms = (uint16_t)(MS_PER_SEC / configTICK_RATE_HZ);
#endif
}

void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told;
    uint32_t tnow;
    uint32_t tcnt = 0;
    uint32_t reload = SysTick->LOAD;
#if USE_FREERTOS
    BaseType_t scheduler_running = pdFALSE;
#endif

    ticks = nus * g_fac_us;

#if USE_FREERTOS
    if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) &&
        (xPortIsInsideInterrupt() == 0))
    {
        scheduler_running = pdTRUE;
        vTaskSuspendAll();
    }
#endif

    told = SysTick->VAL;
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

#if USE_FREERTOS
    if (scheduler_running != pdFALSE)
    {
        (void)xTaskResumeAll();
    }
#endif
}

void delay_ms(uint16_t nms)
{
#if USE_FREERTOS
    if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) &&
        (xPortIsInsideInterrupt() == 0))
    {
        if (nms >= g_fac_ms)
        {
            vTaskDelay((TickType_t)(nms / g_fac_ms));
        }

        nms %= g_fac_ms;
    }
#endif

    delay_us((uint32_t)nms * US_PER_MS);
}

void HAL_Delay(uint32_t Delay)
{
    delay_ms((uint16_t)Delay);
}
