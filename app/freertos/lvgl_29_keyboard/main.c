/**
 * @file    main.c
 * @brief   lvgl_29_keyboard: LVGL lv_keyboard demo (ALIENTEK experiment 29).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"

#define LVGL_TASK_PRIO     3
#define LVGL_TASK_STK_SIZE 1024
#define LED_TASK_PRIO      4
#define LED_TASK_STK_SIZE  128

#define SCR_ACT_WIDTH()    lv_obj_get_width(lv_scr_act())
#define SCR_ACT_HEIGHT()   lv_obj_get_height(lv_scr_act())

/* Toggle the keyboard between number and text-lower mode on the symbol key. */
static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        uint16_t id = lv_btnmatrix_get_selected_btn(target);
        const char *txt = lv_btnmatrix_get_btn_text(target, id);

        if (strcmp(txt, LV_SYMBOL_KEYBOARD) == 0)
        {
            if (lv_keyboard_get_mode(target) == LV_KEYBOARD_MODE_NUMBER)
            {
                lv_keyboard_set_mode(target, LV_KEYBOARD_MODE_TEXT_LOWER);
            }
            else
            {
                lv_keyboard_set_mode(target, LV_KEYBOARD_MODE_NUMBER);
            }
        }
    }
}

static void example_keyboard(void)
{
    lv_obj_t *textarea = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(textarea, SCR_ACT_WIDTH() - 10, SCR_ACT_HEIGHT() / 2 - 10);
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(keyboard, textarea);
    lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
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
    lv_port_disp_init();
    lv_port_indev_init();

    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STK_SIZE, NULL, LVGL_TASK_PRIO, NULL);
    xTaskCreate(led_task, "led", LED_TASK_STK_SIZE, NULL, LED_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
