/**
 * @file    main.c
 * @brief   20_memory: FreeRTOS heap management (ALIENTEK experiment 20).
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

static TaskHandle_t s_start_task;

static void task1(void *argument)
{
    uint8_t *buf = NULL;
    (void)argument;

    for (;;)
    {
        char line[32];
        size_t free_size;

        switch (key_scan(false))
        {
        case KEY0:
            buf = (uint8_t *)pvPortMalloc(30);
            snprintf(line, sizeof(line), "0x%08lx", (unsigned long)(uintptr_t)buf);
            lcd_show_string(130, 160, 200, 16, LCD_FONT_SIZE_16, line, BLUE);
            break;
        case KEY1:
            if (buf != NULL)
            {
                vPortFree(buf);
                buf = NULL;
            }
            break;
        default:
            break;
        }

        snprintf(line, sizeof(line), "Total Mem: %lu Bytes", (unsigned long)configTOTAL_HEAP_SIZE);
        lcd_show_string(30, 118, 240, 16, LCD_FONT_SIZE_16, line, RED);

        free_size = xPortGetFreeHeapSize();
        snprintf(line, sizeof(line), "Free  Mem: %lu Bytes", (unsigned long)free_size);
        lcd_show_string(30, 139, 240, 16, LCD_FONT_SIZE_16, line, RED);

        vTaskDelay(10);
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Mem Manage", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 118, 240, 16, LCD_FONT_SIZE_16, "Total Mem:      Bytes", RED);
    lcd_show_string(30, 139, 240, 16, LCD_FONT_SIZE_16, "Free  Mem:      Bytes", RED);
    lcd_show_string(30, 160, 240, 16, LCD_FONT_SIZE_16, "Malloc Addr:", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
