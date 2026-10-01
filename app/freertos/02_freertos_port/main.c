/**
 * @file    main.c
 * @brief   02_freertos_port: FreeRTOS porting demo (ALIENTEK experiment 2).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "led.h"

#include "FreeRTOS.h"
#include "task.h"

#define START_TASK_PRIO   1U
#define START_STK_SIZE    128U
#define TASK1_PRIO        2U
#define TASK1_STK_SIZE    128U
#define TASK2_PRIO        3U
#define TASK2_STK_SIZE    128U

static const uint32_t s_discolor[11] =
{
    WHITE, BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY
};

static TaskHandle_t s_start_task;

static void task1(void *argument);
static void task2(void *argument);

static void show_title(void)
{
    lcd_show_string(10, 10, 220, 32, LCD_FONT_SIZE_32, "STM32", RED);
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "FreeRTOS Porting", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
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

static void task1(void *argument)
{
    uint32_t n = 0U;

    (void)argument;

    for (;;)
    {
        lcd_clear(s_discolor[++n % 11U]);
        show_title();
        led_toggle(LED0);
        vTaskDelay(1000);
    }
}

static void task2(void *argument)
{
    float f = 0.0f;

    (void)argument;

    for (;;)
    {
        f += 0.01f;
        printf("float_num: %d.%02d\r\n", (int)f, (int)((f - (int)f) * 100.0f));
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

    show_title();

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
