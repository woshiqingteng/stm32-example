/**
 * @file    delay.c
 * @brief   SysTick based delay, following the vendor delay.c structure.
 *
 * SysTick_Handler() is provided here for both builds:
 *   baremetal : only keeps the HAL tick alive (HAL_IncTick).
 *   freertos  : also chains to the RTOS tick once the scheduler is running.
 * delay_us()/delay_ms() are pure SysTick register busy-waits (no scheduler API),
 * so they are safe inside critical sections / ISRs (ALIENTEK style). Use
 * vTaskDelay() when a task should sleep instead of spinning.
 */

#include "stm32f4xx_hal.h"
#include "delay.h"

#define US_PER_MS   1000U

/* Max consecutive SysTick->VAL reads without a change before delay_us() gives
 * up, so a stopped SysTick cannot hang the caller. In normal operation the
 * counter advances within a few reads, so timing is unaffected. */
#define DELAY_US_STALL_LIMIT_COUNT 1000000U

static uint32_t g_fac_us = 0;

#if USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

/* Provided by portable/GCC/ARM_CM4F/port.c (not declared in the public headers). */
void xPortSysTickHandler(void);
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
    /* 1 us = sysclk (MHz) core cycles; bsp_init() calls delay_init(180). */
    g_fac_us = sysclk;
}

void delay_us(uint32_t nus)
{
    uint64_t ticks = (uint64_t)nus * g_fac_us;
    uint32_t reload = SysTick->LOAD;
    uint32_t told = SysTick->VAL;
    uint64_t tcnt = 0U;
    uint32_t stalled = 0U;

    /* Pure SysTick register polling: no scheduler API, so it is safe with
     * interrupts masked (critical sections / ISRs). */
    while (1)
    {
        uint32_t tnow = SysTick->VAL;

        if (tnow != told)
        {
            tcnt += (tnow < told) ? (uint32_t)(told - tnow)
                                  : (uint32_t)(reload - tnow + told);
            told = tnow;
            stalled = 0U;

            if (tcnt >= ticks)
            {
                break;
            }
        }
        else if (++stalled >= DELAY_US_STALL_LIMIT_COUNT)
        {
            break;
        }
    }
}

void delay_ms(uint16_t nms)
{
    /* Busy wait (ALIENTEK style): use vTaskDelay() when the CPU should sleep. */
    delay_us((uint32_t)nms * US_PER_MS);
}

void HAL_Delay(uint32_t Delay)
{
    while (Delay > 0xFFFFU)
    {
        delay_ms(0xFFFFU);
        Delay -= 0xFFFFU;
    }

    delay_ms((uint16_t)Delay);
}
