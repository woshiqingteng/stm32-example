/**
 * @file    main.c
 * @brief   06_3_task_suspend_resume: task suspend/resume (experiment 6-3).
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
#define TASK1_STK_SIZE  128U
#define TASK2_PRIO      3U
#define TASK2_STK_SIZE  128U
#define TASK3_PRIO      4U
#define TASK3_STK_SIZE  128U

static const uint32_t s_discolor[11] =
{
    WHITE, BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY
};

static TaskHandle_t s_task1;
static TaskHandle_t s_start_task;

static void draw_frame(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t c)
{
    lcd_fill(x1, y1, x2, y1, c);
    lcd_fill(x1, y2, x2, y2, c);
    lcd_fill(x1, y1, x1, y2, c);
    lcd_fill(x2, y1, x2, y2, c);
}

static void show_num3(uint16_t x, uint16_t y, uint32_t n, uint32_t c)
{
    char b[8];
    snprintf(b, sizeof(b), "%03lu", (unsigned long)n);
    lcd_show_string(x, y, 60, 16, LCD_FONT_SIZE_16, b, c);
}

static void task1(void *argument)
{
    uint32_t n = 0U;
    (void)argument;

    for (;;)
    {
        lcd_fill(6, 131, 114, 313, s_discolor[++n % 11U]);
        show_num3(71, 111, n, BLUE);
        vTaskDelay(500);
    }
}

static void task2(void *argument)
{
    uint32_t n = 0U;
    (void)argument;

    for (;;)
    {
        lcd_fill(126, 131, 233, 313, s_discolor[11U - (++n % 11U)]);
        show_num3(191, 111, n, BLUE);
        vTaskDelay(500);
    }
}

static void task3(void *argument)
{
    (void)argument;

    for (;;)
    {
        switch (key_scan(false))
        {
        case KEY0:
            vTaskSuspend(s_task1);
            break;
        case KEY1:
            vTaskResume(s_task1);
            break;
        default:
            break;
        }
        vTaskDelay(10);
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
    (void)xTaskCreate(task1, "task1", TASK1_STK_SIZE, NULL, TASK1_PRIO, &s_task1);
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Task Susp & Resum", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    draw_frame(5, 110, 115, 314, BLACK);
    draw_frame(125, 110, 234, 314, BLACK);
    lcd_show_string(15, 111, 110, 16, LCD_FONT_SIZE_16, "Task1: 000", BLUE);
    lcd_show_string(135, 111, 110, 16, LCD_FONT_SIZE_16, "Task2: 000", BLUE);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
