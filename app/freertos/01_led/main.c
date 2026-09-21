/**
 * @file    main.c
 * @brief   01_led: minimal FreeRTOS task demo (LED blink + serial print).
 */

#include <stdio.h>
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_TASK_STACK    128U
#define PRINT_TASK_STACK  256U
#define LED_TASK_PRIO     2U
#define PRINT_TASK_PRIO   1U

#define LED_PERIOD_MS     500U
#define PRINT_PERIOD_MS   1000U

static void led_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        led_toggle(LED0);
        vTaskDelay(pdMS_TO_TICKS(LED_PERIOD_MS));
    }
}

static void print_task(void *argument)
{
    uint32_t counter = 0U;

    (void)argument;

    for (;;)
    {
        printf("freertos 01_led: tick %lu\r\n", (unsigned long)counter);
        counter++;
        vTaskDelay(pdMS_TO_TICKS(PRINT_PERIOD_MS));
    }
}

int main(void)
{
    bsp_init();
    printf("01_led (FreeRTOS) ready\r\n");

    (void)xTaskCreate(led_task, "led", LED_TASK_STACK, NULL, LED_TASK_PRIO, NULL);
    (void)xTaskCreate(print_task, "print", PRINT_TASK_STACK, NULL, PRINT_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
