/**
 * @file    main.c
 * @brief   lvgl_04_mouse: official LVGL stress demo with the touch pointer
 *          (ALIENTEK experiment 4, "add mouse").
 */

#include <stdio.h>

#include "bsp.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"
#include "demos/lv_demos.h"

#define LVGL_TASK_PRIO     3
#define LVGL_TASK_STK_SIZE 2048
#define LED_TASK_PRIO      4
#define LED_TASK_STK_SIZE  128

static void lvgl_task(void *pvParameters)
{
    (void)pvParameters;

    lv_demo_stress();

    for (;;)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void led_task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        led_toggle(LED0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    sdram_init();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STK_SIZE, NULL, LVGL_TASK_PRIO, NULL);
    xTaskCreate(led_task, "led", LED_TASK_STK_SIZE, NULL, LED_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
