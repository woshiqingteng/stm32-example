/**
 * @file    main.c
 * @brief   16_event_group: FreeRTOS event groups (ALIENTEK experiment 16).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  256U
#define TASK2_PRIO      4U
#define TASK2_STK_SIZE  256U
#define TASK3_PRIO      3U
#define TASK3_STK_SIZE  256U

#define EVENTBIT_0      (1UL << 0)
#define EVENTBIT_1      (1UL << 1)
#define EVENTBIT_ALL    (EVENTBIT_0 | EVENTBIT_1)

static const uint32_t s_discolor[11] =
{
    WHITE, BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY
};

static EventGroupHandle_t s_event;
static TaskHandle_t       s_start_task;

static void task1(void *argument)
{
    (void)argument;

    for (;;)
    {
        switch (key_scan(false))
        {
        case KEY0:
            (void)xEventGroupSetBits(s_event, EVENTBIT_0);
            break;
        case KEY1:
            (void)xEventGroupSetBits(s_event, EVENTBIT_1);
            break;
        default:
            break;
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
        (void)xEventGroupWaitBits(s_event, EVENTBIT_ALL, pdTRUE, pdTRUE, portMAX_DELAY);
        lcd_fill(6, 131, 233, 313, s_discolor[++n % 11U]);
        vTaskDelay(10);
    }
}

static void task3(void *argument)
{
    (void)argument;

    for (;;)
    {
        EventBits_t v = xEventGroupGetBits(s_event);
        char b[24];
        snprintf(b, sizeof(b), "Event Group Value: %lu", (unsigned long)v);
        lcd_show_string(30, 110, 220, 16, LCD_FONT_SIZE_16, b, BLUE);
        vTaskDelay(10);
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
    s_event = xEventGroupCreate();
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Event Group", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_fill(5, 130, 5, 314, BLACK);
    lcd_fill(234, 130, 234, 314, BLACK);
    lcd_fill(5, 130, 234, 130, BLACK);
    lcd_fill(5, 314, 234, 314, BLACK);
    lcd_show_string(30, 110, 220, 16, LCD_FONT_SIZE_16, "Event Group Value: 0", BLUE);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
