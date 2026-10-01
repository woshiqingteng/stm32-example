/**
 * @file    main.c
 * @brief   19_idle_hook: FreeRTOS idle hook (ALIENTEK experiment 19).
 */

#include <stdio.h>

#include "stm32f4xx_hal.h"

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "led.h"
#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"

#define START_TASK_PRIO 1U
#define START_STK_SIZE  128U
#define TASK1_PRIO      2U
#define TASK1_STK_SIZE  256U

static TaskHandle_t s_start_task;

static void idle_sleep(void)
{
    __disable_irq();
    __DSB();
    __ISB();

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOD_CLK_DISABLE();
    __HAL_RCC_GPIOE_CLK_DISABLE();
    __HAL_RCC_GPIOF_CLK_DISABLE();
    __HAL_RCC_GPIOG_CLK_DISABLE();

    __WFI();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    __DSB();
    __ISB();
    __enable_irq();
}

void vApplicationIdleHook(void)
{
    idle_sleep();
}

static void task1(void *argument)
{
    (void)argument;

    for (;;)
    {
        led_off(LED0);          /* busy: no low-power mode (LED off) */
        delay_ms(3000);         /* busy wait (no low-power mode) */
        led_on(LED0);           /* idle: enter low-power mode (LED on) */
        vTaskDelay(3000);
    }
}

static void start_task(void *argument)
{
    (void)argument;

    taskENTER_CRITICAL();
    /* Blank the panel (LTDC off + backlight off) so the idle-hook demo shows
     * only the LED, like the ALIENTEK reference. */
    lcd_display_off();
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
    lcd_show_string(10, 47, 220, 24, LCD_FONT_SIZE_24, "FreeRTOS IDLE HOOK", RED);
    lcd_show_string(10, 76, 220, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    (void)xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &s_start_task);
    vTaskStartScheduler();

    for (;;)
    {
    }
}
