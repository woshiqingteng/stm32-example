/**
 * @file    main.c
 * @brief   04_interrupt: FreeRTOS interrupt-priority test (ALIENTEK experiment 4).
 *
 * Two timers straddle configMAX_SYSCALL_INTERRUPT_PRIORITY (5):
 *   TIM3 update, NVIC pre-emption 4 -> above the threshold (NOT masked)
 *   TIM6 update, NVIC pre-emption 6 -> below the threshold (masked)
 * The reference uses TIM3/TIM5; here TIM3 (gtim) + TIM6 (btim) are used with
 * the same period and pre-emption priorities.
 */

#include <stdio.h>

#include "stm32f4xx_hal.h"

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "delay.h"
#include "btim.h"
#include "gtim.h"

#include "FreeRTOS.h"
#include "task.h"

#define TIM_ARR         9999U
#define TIM_PSC         8999U
#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  256U

static volatile uint32_t s_tim3_count;
static volatile uint32_t s_tim6_count;
static TaskHandle_t s_start_task;

static void on_tim3(void)
{
    s_tim3_count++;
}

static void on_tim6(void)
{
    s_tim6_count++;
}

static void start_task(void *argument);
static void task1(void *argument);

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();

    /* Same period as the reference; priorities copied from the reference. */
    gtim_timx_int_init(TIM_ARR, TIM_PSC);          /* TIM3 */
    btim_timx_int_init(TIM_ARR, TIM_PSC);          /* TIM6 */
    gtim_timx_int_register(on_tim3);
    btim_timx_int_register(on_tim6);
    HAL_NVIC_SetPriority(TIM3_IRQn, 4, 0);         /* above MAX_SYSCALL (5) */
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 6, 0);     /* below MAX_SYSCALL (5) */

    (void)xTaskCreate(task1, "task1", TASK1_STK_SIZE, NULL, TASK1_PRIO, NULL);
    vTaskDelete(s_start_task);
    taskEXIT_CRITICAL();
}

static void task1(void *argument)
{
    uint32_t n = 0U;

    (void)argument;

    for (;;)
    {
        if (++n == 5U)
        {
            printf("FreeRTOS: disable interrupts\r\n");
            portDISABLE_INTERRUPTS();
            /* delay_ms() is a pure busy wait (ALIENTEK style, no scheduler API):
             * vTaskDelay() is illegal while the tick/PendSV are masked. TIM3
             * (pre-emption 4) keeps firing; TIM6 (pre-emption 6) is masked by
             * configMAX_SYSCALL_INTERRUPT_PRIORITY (5). */
            delay_ms(5000);
            printf("FreeRTOS: enable interrupts\r\n");
            portENABLE_INTERRUPTS();
        }
        printf("tim3=%lu tim6=%lu\r\n", (unsigned long)s_tim3_count, (unsigned long)s_tim6_count);
        vTaskDelay(1000);
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    sdram_init();
    lcd_init();
    lcd_display_dir(LCD_DIR_PORTRAIT);
    lcd_clear(WHITE);

    lcd_show_string(10, 10, 220, 32, LCD_FONT_SIZE_32, "STM32", RED);
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Interrupt", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
