/**
 * @file    main.c
 * @brief   09_time_slicing: round-robin time slicing (ALIENTEK experiment 9).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  256U
#define TASK2_PRIO      2U
#define TASK2_STK_SIZE  256U

static TaskHandle_t s_start_task;

static void task1(void *argument)
{
    uint32_t n = 0U;
    (void)argument;

    for (;;)
    {
        taskENTER_CRITICAL();
        printf("task1 run count: %lu\r\n", (unsigned long)(++n));
        taskEXIT_CRITICAL();
    }
}

static void task2(void *argument)
{
    uint32_t n = 0U;
    (void)argument;

    for (;;)
    {
        taskENTER_CRITICAL();
        printf("task2 run count: %lu\r\n", (unsigned long)(++n));
        taskEXIT_CRITICAL();
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
    (void)xTaskCreate(task1, "task1", TASK1_STK_SIZE, NULL, TASK1_PRIO, NULL);
    (void)xTaskCreate(task2, "task2", TASK2_STK_SIZE, NULL, TASK2_PRIO, NULL);
    vTaskDelete(s_start_task);
    taskEXIT_CRITICAL();
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "FreeRTOS Round Robin", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
