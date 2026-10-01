/**
 * @file    main.c
 * @brief   14_3_priority_inversion: priority inversion with a binary semaphore
 *          (ALIENTEK experiment 14-3).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      4U
#define TASK1_STK_SIZE  256U
#define TASK2_PRIO      3U
#define TASK2_STK_SIZE  256U
#define TASK3_PRIO      2U
#define TASK3_STK_SIZE  256U

static SemaphoreHandle_t s_sem;
static TaskHandle_t      s_start_task;

static void task1(void *argument)
{
    (void)argument;

    vTaskDelay(500);
    for (;;)
    {
        printf("task1 ready to take sem\r\n");
        (void)xSemaphoreTake(s_sem, portMAX_DELAY);
        printf("task1 has taken sem, running\r\n");
        printf("task1 give sem\r\n");
        (void)xSemaphoreGive(s_sem);
        vTaskDelay(100);
    }
}

static void task2(void *argument)
{
    uint32_t i;
    (void)argument;

    vTaskDelay(200);
    for (;;)
    {
        for (i = 0U; i < 5U; i++)
        {
            printf("task2 running\r\n");
            delay_ms(100);
        }
        vTaskDelay(1000);
    }
}

static void task3(void *argument)
{
    uint32_t i;
    (void)argument;

    for (;;)
    {
        printf("task3 ready to take sem\r\n");
        (void)xSemaphoreTake(s_sem, portMAX_DELAY);
        printf("task3 has taken sem\r\n");
        for (i = 0U; i < 5U; i++)
        {
            printf("task3 running\r\n");
            delay_ms(100);
        }
        printf("task3 give sem\r\n");
        (void)xSemaphoreGive(s_sem);
        vTaskDelay(1000);
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
    s_sem = xSemaphoreCreateBinary();
    (void)xTaskCreate(task1, "task1", TASK1_STK_SIZE, NULL, TASK1_PRIO, NULL);
    (void)xTaskCreate(task2, "task2", TASK2_STK_SIZE, NULL, TASK2_PRIO, NULL);
    (void)xTaskCreate(task3, "task3", TASK3_STK_SIZE, NULL, TASK3_PRIO, NULL);
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Priority Inversion", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
