/**
 * @file    main.c
 * @brief   lv_29_keyboard: LVGL lv_keyboard demo (textarea + keyboard).
 */

#include <stdio.h>

#include "bsp.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_port_tick.h"

#define LVGL_TASK_PRIO     3
#define LVGL_TASK_STK_SIZE 1024
#define LED_TASK_PRIO      4
#define LED_TASK_STK_SIZE  128

/* Mode switching is handled by the built-in keys "1#" / "abc" / "ABC". */
static void example_keyboard(void)
{
    lv_obj_t *textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(textarea, lv_obj_get_width(lv_scr_act()) - 10, lv_obj_get_height(lv_scr_act()) / 2 - 10);
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(keyboard, textarea);
}

static void lvgl_task(void *pvParameters)
{
    (void)pvParameters;

    example_keyboard();

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
    lv_port_tick_init();
    lv_port_disp_init();
    lv_port_indev_init();

    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STK_SIZE, NULL, LVGL_TASK_PRIO, NULL);
    xTaskCreate(led_task, "led", LED_TASK_STK_SIZE, NULL, LED_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
