/**
 * @file    main.c
 * @brief   07_list_item: FreeRTOS list/list-item insert & remove (experiment 7).
 */

#include <stdint.h>
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

static TaskHandle_t s_start_task;
static List_t       s_list;
static ListItem_t   s_item1;
static ListItem_t   s_item2;
static ListItem_t   s_item3;

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

    vListInitialise(&s_list);
    vListInitialiseItem(&s_item1);
    vListInitialiseItem(&s_item2);
    vListInitialiseItem(&s_item3);

    printf("== list ready ==\r\n");
    printf("TestList     %08lx\r\n", (unsigned long)(uintptr_t)&s_list);
    printf("ListItem1    %08lx\r\n", (unsigned long)(uintptr_t)&s_item1);
    printf("ListItem2    %08lx\r\n", (unsigned long)(uintptr_t)&s_item2);
    printf("ListItem3    %08lx\r\n", (unsigned long)(uintptr_t)&s_item3);
    wait_key0();

    vListInsert(&s_list, &s_item1);
    printf("insert item1: listEnd.next=%08lx item1.next=%08lx\r\n",
           (unsigned long)(uintptr_t)s_list.xListEnd.pxNext,
           (unsigned long)(uintptr_t)s_item1.pxNext);
    wait_key0();

    vListInsert(&s_list, &s_item2);
    printf("insert item2: listEnd.next=%08lx item2.next=%08lx\r\n",
           (unsigned long)(uintptr_t)s_list.xListEnd.pxNext,
           (unsigned long)(uintptr_t)s_item2.pxNext);
    wait_key0();

    vListInsert(&s_list, &s_item3);
    printf("insert item3: listEnd.next=%08lx item3.next=%08lx\r\n",
           (unsigned long)(uintptr_t)s_list.xListEnd.pxNext,
           (unsigned long)(uintptr_t)s_item3.pxNext);
    wait_key0();

    (void)uxListRemove(&s_item2);
    printf("remove item2: listEnd.next=%08lx item1.next=%08lx\r\n",
           (unsigned long)(uintptr_t)s_list.xListEnd.pxNext,
           (unsigned long)(uintptr_t)s_item1.pxNext);
    wait_key0();

    vListInsertEnd(&s_list, &s_item2);
    printf("insert item2 at end: done\r\n");
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "List & ListItem", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
