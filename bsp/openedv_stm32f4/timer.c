/**
 * @file    timer.c
 * @brief   Camera frame-rate timer: TIM6 update interrupt at 1 Hz.
 *
 * TIM6 hangs off APB1; with a 45 MHz APB1 clock the timer kernel runs at
 * 90 MHz, so (prescaler + 1) * (period + 1) = 90e6 gives a 1 s interval. The
 * update ISR latches the frame count into the frame-rate variable and prints
 * it on USART1, mirroring the vendor frame counter.
 */

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "timer.h"

#define TIMER_TIMX                     TIM6
#define TIMER_TIMX_IRQN                TIM6_DAC_IRQn
#define TIMER_TIMX_IRQ_PREEMPT_PRIO    1U
#define TIMER_TIMX_IRQ_SUB_PRIO        3U

/* 90 MHz / (9000 * 10000) = 1 Hz. */
#define TIMER_PRESCALER                9000U
#define TIMER_PERIOD                   10000U

static TIM_HandleTypeDef g_timer_handle;

static volatile uint32_t g_frame_count;
static volatile uint32_t g_frame_total;
static volatile uint32_t g_frame_rate;
static volatile uint32_t g_uptime;

void timer_init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    HAL_NVIC_SetPriority(TIMER_TIMX_IRQN, TIMER_TIMX_IRQ_PREEMPT_PRIO, TIMER_TIMX_IRQ_SUB_PRIO);
    HAL_NVIC_EnableIRQ(TIMER_TIMX_IRQN);

    g_timer_handle.Instance          = TIMER_TIMX;
    g_timer_handle.Init.Prescaler    = TIMER_PRESCALER - 1U;
    g_timer_handle.Init.CounterMode  = TIM_COUNTERMODE_UP;
    g_timer_handle.Init.Period       = TIMER_PERIOD - 1U;
    g_timer_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    (void)HAL_TIM_Base_Init(&g_timer_handle);
    (void)HAL_TIM_Base_Start_IT(&g_timer_handle);
}

void timer_frame_inc(void)
{
    g_frame_count++;
    g_frame_total++;
}

uint32_t timer_frame_count(void)
{
    return g_frame_total;
}

uint32_t timer_frame_rate(void)
{
    return g_frame_rate;
}

uint32_t timer_uptime(void)
{
    return g_uptime;
}

void TIM6_DAC_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_timer_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_timer_handle, TIM_FLAG_UPDATE);

        g_frame_rate = g_frame_count;
        g_frame_count = 0U;
        g_uptime++;

        printf("frame:%u\r\n", (unsigned int)g_frame_rate);
    }
}
