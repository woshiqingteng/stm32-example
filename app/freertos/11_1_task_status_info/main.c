/**
 * @file    main.c
 * @brief   11_1_task_status_info: task state/info queries (experiment 11-1).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "malloc.h"

#include "FreeRTOS.h"
#include "task.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  256U

static TaskHandle_t s_start_task;

static void wait_key0(void)
{
    printf("press KEY0 to continue...\r\n");
    while (key_scan(false) != KEY0)
    {
        vTaskDelay(10);
    }
}

static void task1(void *argument)
{
    (void)argument;

    /* uxTaskGetSystemState() */
    UBaseType_t tcount = uxTaskGetNumberOfTasks();
    TaskStatus_t *arr = (TaskStatus_t *)mymalloc(SRAMIN, tcount * sizeof(TaskStatus_t));

    printf("== uxTaskGetSystemState ==\r\n");
    if (arr != NULL)
    {
        uint32_t i;
        tcount = uxTaskGetSystemState(arr, tcount, NULL);
        printf("name\tprio\tnumber\r\n");
        for (i = 0U; i < tcount; i++)
        {
            printf("%s\t%ld\t%ld\r\n", arr[i].pcTaskName,
                   (unsigned long)arr[i].uxCurrentPriority, (unsigned long)arr[i].xTaskNumber);
        }
        myfree(SRAMIN, arr);
    }
    wait_key0();

    /* vTaskGetInfo() */
    {
        TaskStatus_t *info = (TaskStatus_t *)mymalloc(SRAMIN, sizeof(TaskStatus_t));
        TaskHandle_t h = xTaskGetHandle("task1");
        printf("== vTaskGetInfo(task1) ==\r\n");
        if (info != NULL)
        {
            vTaskGetInfo(h, info, pdTRUE, eInvalid);
            printf("name=%s number=%ld state=%d prio=%ld base=%ld highwater=%u\r\n",
                   info->pcTaskName, (unsigned long)info->xTaskNumber, (int)info->eCurrentState,
                   (unsigned long)info->uxCurrentPriority, (unsigned long)info->uxBasePriority,
                   (unsigned)info->usStackHighWaterMark);
            myfree(SRAMIN, info);
        }
    }
    wait_key0();

    /* eTaskGetState() */
    {
        eTaskState st = eTaskGetState(xTaskGetHandle("task1"));
        const char *s = (st == eRunning) ? "Running" : (st == eReady) ? "Ready" :
                        (st == eBlocked) ? "Blocked" : (st == eSuspended) ? "Suspended" :
                        (st == eDeleted) ? "Deleted" : "Invalid";
        printf("== eTaskGetState(task1) == state %d (%s)\r\n", (int)st, s);
    }
    wait_key0();

    /* vTaskList() */
    {
        char *buf = (char *)mymalloc(SRAMIN, 500);
        printf("== vTaskList ==\r\n");
        if (buf != NULL)
        {
            vTaskList(buf);
            printf("%s\r\n", buf);
            myfree(SRAMIN, buf);
        }
    }
    printf("== demo finished ==\r\n");

    for (;;)
    {
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "Task Info Query", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
