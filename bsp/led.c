/**
 * @file    led.c
 * @brief   On-board LED driver (active low).
 */

#include "stm32f4xx_hal.h"
#include "led.h"

#define LED_GPIO_PORT          GPIOB
#define LED_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
} led_hw_t;

static const led_hw_t g_led_hw[] =
{
    { GPIOB, GPIO_PIN_1 },  /* LED0 (RED)   */
    { GPIOB, GPIO_PIN_0 },  /* LED1 (GREEN) */
};

#define LED_NUM (sizeof(g_led_hw) / sizeof(g_led_hw[0]))

static const led_hw_t *led_hw_get(led_id_t id)
{
    if ((uint32_t)id >= (uint32_t)LED_NUM)
    {
        return 0;
    }

    return &g_led_hw[id];
}

void led_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    LED_GPIO_CLK_ENABLE();

    gpio_init.Pin   = g_led_hw[LED0].pin | g_led_hw[LED1].pin;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio_init);

    led_off(LED0);
    led_off(LED1);
}

void led_on(led_id_t id)
{
    const led_hw_t *hw = led_hw_get(id);

    if (hw != 0)
    {
        HAL_GPIO_WritePin(hw->port, hw->pin, GPIO_PIN_RESET);
    }
}

void led_off(led_id_t id)
{
    const led_hw_t *hw = led_hw_get(id);

    if (hw != 0)
    {
        HAL_GPIO_WritePin(hw->port, hw->pin, GPIO_PIN_SET);
    }
}

void led_toggle(led_id_t id)
{
    const led_hw_t *hw = led_hw_get(id);

    if (hw != 0)
    {
        HAL_GPIO_TogglePin(hw->port, hw->pin);
    }
}
