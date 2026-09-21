/**
 * @file    delay.c
 * @brief   SysTick based delay, following the vendor delay.c structure.
 *
 * The OS branch uses FreeRTOS primitives (vTaskSuspendAll/xTaskResumeAll,
 * vTaskDelay, xPortIsInsideInterrupt). The 1 ms SysTick interrupt stays enabled;
 * SysTick_Handler() chains to the RTOS tick when the scheduler is running.
 */

#include "stm32f4xx_hal.h"
#include "delay.h"

static uint32_t g_fac_us = 0;

#if USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

/* Provided by portable/GCC/ARM_CM4F/port.c (not declared in the public headers). */
void xPortSysTickHandler(void);

static uint16_t g_fac_ms = 0;

#define delay_osrunning (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)

static void delay_osschedlock(void)
{
    vTaskSuspendAll();
}

static void delay_osschedunlock(void)
{
    (void)xTaskResumeAll();
}

static void delay_ostimedly(uint32_t ticks)
{
    vTaskDelay(ticks);
}

void SysTick_Handler(void)
{
    HAL_IncTick();

    if (delay_osrunning)
    {
        xPortSysTickHandler();
    }
}
#endif

void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk;

#if USE_FREERTOS
    g_fac_ms = (uint16_t)(1000U / configTICK_RATE_HZ);
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
    BaseType_t sched = delay_osrunning;
#endif

    ticks = nus * g_fac_us;

#if USE_FREERTOS
    if (sched)
    {
        delay_osschedlock();
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
    if (sched)
    {
        delay_osschedunlock();
    }
#endif
}

void delay_ms(uint16_t nms)
{
#if USE_FREERTOS
    if (delay_osrunning && (xPortIsInsideInterrupt() == 0))
    {
        if (nms >= g_fac_ms)
        {
            delay_ostimedly((uint32_t)(nms / g_fac_ms));
        }

        nms %= g_fac_ms;
    }
#endif

    delay_us((uint32_t)nms * 1000U);
}

void HAL_Delay(uint32_t Delay)
{
    delay_ms((uint16_t)Delay);
}
