/**
 * @file    main.c
 * @brief   15_software_timer: FreeRTOS software timers (ALIENTEK experiment 15).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  128U

static const uint32_t s_discolor[11] =
{
    WHITE, BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY
};

static TimerHandle_t s_timer1;
static TimerHandle_t s_timer2;
static TaskHandle_t  s_start_task;

static void show_num3(uint16_t x, uint16_t y, uint32_t n)
{
    char b[8];
    snprintf(b, sizeof(b), "%03lu", (unsigned long)n);
    lcd_show_string(x, y, 60, 16, LCD_FONT_SIZE_16, b, BLUE);
}

static void timer1_cb(TimerHandle_t xTimer)
{
    static uint32_t n = 0U;
    (void)xTimer;
    lcd_fill(6, 131, 114, 313, s_discolor[++n % 11U]);
    show_num3(79, 111, n);
}

static void timer2_cb(TimerHandle_t xTimer)
{
    static uint32_t n = 0U;
    (void)xTimer;
    lcd_fill(126, 131, 233, 313, s_discolor[++n % 11U]);
    show_num3(199, 111, n);
}

static void task1(void *argument)
{
    (void)argument;

    for (;;)
    {
        switch (key_scan(false))
        {
        case KEY0:
            (void)xTimerStart(s_timer1, portMAX_DELAY);
            (void)xTimerStart(s_timer2, portMAX_DELAY);
            break;
        case KEY1:
            (void)xTimerStop(s_timer1, portMAX_DELAY);
            (void)xTimerStop(s_timer2, portMAX_DELAY);
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
    s_timer1 = xTimerCreate("Timer1", 1000, pdTRUE, (void *)1, timer1_cb);
    s_timer2 = xTimerCreate("Timer2", 1000, pdFALSE, (void *)2, timer2_cb);
    (void)xTaskCreate(task1, "task1", TASK1_STK_SIZE, NULL, TASK1_PRIO, NULL);
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Timer", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    lcd_fill(5, 110, 5, 314, BLACK);
    lcd_fill(115, 110, 115, 314, BLACK);
    lcd_fill(125, 110, 125, 314, BLACK);
    lcd_fill(234, 110, 234, 314, BLACK);
    lcd_fill(5, 110, 115, 110, BLACK);
    lcd_fill(125, 110, 234, 110, BLACK);
    lcd_fill(5, 130, 115, 130, BLACK);
    lcd_fill(125, 130, 234, 130, BLACK);
    lcd_fill(5, 314, 234, 314, BLACK);
    lcd_show_string(15, 111, 110, 16, LCD_FONT_SIZE_16, "Timer1: 000", BLUE);
    lcd_show_string(135, 111, 110, 16, LCD_FONT_SIZE_16, "Timer2: 000", BLUE);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
