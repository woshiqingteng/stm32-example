/**
 * @file    led.c
 * @brief   On-board LED driver (active low).
 */

#include "stm32f4xx_hal.h"
#include "led.h"

#define LED0_GPIO_PORT         GPIOB
#define LED0_GPIO_PIN          GPIO_PIN_1
#define LED1_GPIO_PORT         GPIOB
#define LED1_GPIO_PIN          GPIO_PIN_0

#define LED_GPIO_PORT          GPIOB
#define LED_GPIO_PINS          (LED0_GPIO_PIN | LED1_GPIO_PIN)
#define LED_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

void led_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    LED_GPIO_CLK_ENABLE();

    gpio_init.Pin   = LED_GPIO_PINS;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio_init);

    led_off(LED0);
    led_off(LED1);
}

void led_on(led_id_t id)
{
    switch (id)
    {
        case LED0:
            HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_RESET);
            break;
        case LED1:
            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);
            break;
        default:
            break;
    }
}

void led_off(led_id_t id)
{
    switch (id)
    {
        case LED0:
            HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_SET);
            break;
        case LED1:
            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET);
            break;
        default:
            break;
    }
}

void led_toggle(led_id_t id)
{
    switch (id)
    {
        case LED0:
            HAL_GPIO_TogglePin(LED0_GPIO_PORT, LED0_GPIO_PIN);
            break;
        case LED1:
            HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_GPIO_PIN);
            break;
        default:
            break;
    }
}
