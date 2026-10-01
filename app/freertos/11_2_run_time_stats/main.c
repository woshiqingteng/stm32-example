/**
 * @file    main.c
 * @brief   11_2_run_time_stats: task run-time statistics (experiment 11-2).
 *
 * A basic timer (TIM6) provides the high-resolution counter used by
 * configGENERATE_RUN_TIME_STATS (see FreeRTOSConfig.h).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "malloc.h"
#include "btim.h"

#include "FreeRTOS.h"
#include "task.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  128U
#define TASK2_PRIO      3U
#define TASK2_STK_SIZE  128U
#define TASK3_PRIO      4U
#define TASK3_STK_SIZE  256U

static const uint32_t s_discolor[11] =
{
    WHITE, BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY
};

volatile uint32_t g_rtos_run_ticks;

static void run_ticks_cb(void)
{
    g_rtos_run_ticks++;
}

void rtos_runtime_timer_init(void)
{
    btim_timx_int_register(run_ticks_cb);
    /* 90 MHz / ((899+1)*(9+1)) = 10 kHz */
    btim_timx_int_init(9U, 899U);
}

static TaskHandle_t s_start_task;

static void show_num3(uint16_t x, uint16_t y, uint32_t n)
{
    char b[8];
    snprintf(b, sizeof(b), "%03lu", (unsigned long)n);
    lcd_show_string(x, y, 60, 16, LCD_FONT_SIZE_16, b, BLUE);
}

static void task1(void *argument)
{
    uint32_t n = 0U;
    (void)argument;

    for (;;)
    {
        lcd_fill(6, 131, 114, 313, s_discolor[++n % 11U]);
        show_num3(71, 111, n);
        vTaskDelay(1000);
    }
}

static void task2(void *argument)
{
    uint32_t n = 0U;
    (void)argument;

    for (;;)
    {
        lcd_fill(126, 131, 233, 313, s_discolor[11U - (++n % 11U)]);
        show_num3(191, 111, n);
        vTaskDelay(1000);
    }
}

static void task3(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            char *buf = (char *)mymalloc(SRAMIN, 100);
            printf("name\t\truntime\tpercent\r\n");
            if (buf != NULL)
            {
                vTaskGetRunTimeStats(buf);
                printf("%s\r\n", buf);
                myfree(SRAMIN, buf);
            }
        }
        vTaskDelay(10);
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Get Run Time Stats", RED);
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
    lcd_show_string(15, 111, 110, 16, LCD_FONT_SIZE_16, "Task1: 000", BLUE);
    lcd_show_string(135, 111, 110, 16, LCD_FONT_SIZE_16, "Task2: 000", BLUE);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
