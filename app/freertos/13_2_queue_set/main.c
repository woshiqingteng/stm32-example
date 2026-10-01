/**
 * @file    main.c
 * @brief   13_2_queue_set: FreeRTOS queue set (ALIENTEK experiment 13-2).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  256U
#define TASK2_PRIO      3U
#define TASK2_STK_SIZE  256U

#define QUEUE_LENGTH            1U
#define QUEUE_ITEM_SIZE         sizeof(uint32_t)
#define SEMAPHORE_BINARY_LENGTH 1U
#define QUEUESET_LENGTH         ((2U * QUEUE_LENGTH) + SEMAPHORE_BINARY_LENGTH)

static QueueSetHandle_t  s_queue_set;
static QueueHandle_t     s_queue1;
static QueueHandle_t     s_queue2;
static SemaphoreHandle_t s_semaphore;
static TaskHandle_t      s_start_task;

static void task1(void *argument)
{
    (void)argument;

    for (;;)
    {
        key_id_t key = key_scan(false);
        uint32_t v = (uint32_t)key;

        switch (key)
        {
        case KEY_WKUP:
            (void)xQueueSend(s_queue1, &v, portMAX_DELAY);
            break;
        case KEY1:
            (void)xQueueSend(s_queue2, &v, portMAX_DELAY);
            break;
        case KEY0:
            (void)xSemaphoreGive(s_semaphore);
            break;
        default:
            break;
        }
        vTaskDelay(10);
    }
}

static void task2(void *argument)
{
    (void)argument;

    for (;;)
    {
        QueueSetMemberHandle_t member = xQueueSelectFromSet(s_queue_set, portMAX_DELAY);
        uint32_t v = 0U;

        if (member == s_queue1)
        {
            (void)xQueueReceive(member, &v, portMAX_DELAY);
            printf("queue1 message: %lu\r\n", (unsigned long)v);
        }
        else if (member == s_queue2)
        {
            (void)xQueueReceive(member, &v, portMAX_DELAY);
            printf("queue2 message: %lu\r\n", (unsigned long)v);
        }
        else if (member == s_semaphore)
        {
            (void)xSemaphoreTake(member, portMAX_DELAY);
            printf("binary semaphore taken\r\n");
        }
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
    s_queue_set = xQueueCreateSet(QUEUESET_LENGTH);
    s_queue1 = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    s_queue2 = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    s_semaphore = xSemaphoreCreateBinary();
    (void)xQueueAddToSet(s_queue1, s_queue_set);
    (void)xQueueAddToSet(s_queue2, s_queue_set);
    (void)xQueueAddToSet(s_semaphore, s_queue_set);
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Queue set", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
