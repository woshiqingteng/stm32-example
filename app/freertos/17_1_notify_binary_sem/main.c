/**
 * @file    main.c
 * @brief   17_1_notify_binary_sem: task notification as binary semaphore (17-1).
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
#define TASK2_PRIO      3U
#define TASK2_STK_SIZE  256U

static const uint32_t s_discolor[11] =
{
    WHITE, BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY
};

static TaskHandle_t s_task2;
static TaskHandle_t s_start_task;

static void task1(void *argument)
{
    (void)argument;

    for (;;)
    {
        if ((s_task2 != NULL) && (key_scan(false) == KEY0))
        {
            (void)xTaskNotifyGive(s_task2);
        }
        vTaskDelay(10);
    }
}

static void task2(void *argument)
{
    uint32_t n = 0U;
    (void)argument;

    for (;;)
    {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0U)
        {
            lcd_fill(6, 131, 233, 313, s_discolor[++n % 11U]);
        }
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
    (void)xTaskCreate(task1, "task1", TASK1_STK_SIZE, NULL, TASK1_PRIO, NULL);
    (void)xTaskCreate(task2, "task2", TASK2_STK_SIZE, NULL, TASK2_PRIO, &s_task2);
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Notify Bina Sem", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_fill(5, 130, 5, 314, BLACK);
    lcd_fill(234, 130, 234, 314, BLACK);
    lcd_fill(5, 130, 234, 130, BLACK);
    lcd_fill(5, 314, 234, 314, BLACK);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
