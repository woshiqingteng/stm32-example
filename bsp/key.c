/**
 * @file    key.c
 * @brief   On-board key driver.
 */

#include "stm32f4xx_hal.h"
#include "key.h"
#include "delay.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       active_low;
} key_hw_t;

static const key_hw_t g_key_hw[KEY_NUM] =
{
    { GPIOH, GPIO_PIN_3,  1U },  /* KEY0  */
    { GPIOH, GPIO_PIN_2,  1U },  /* KEY1  */
    { GPIOC, GPIO_PIN_13, 1U },  /* KEY2  */
    { GPIOA, GPIO_PIN_0,  0U },  /* WK_UP */
};

void key_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint32_t i;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    for (i = 0; i < KEY_NUM; i++)
    {
        gpio_init.Pin  = g_key_hw[i].pin;
        gpio_init.Mode = GPIO_MODE_INPUT;
        gpio_init.Pull = g_key_hw[i].active_low ? GPIO_PULLUP : GPIO_PULLDOWN;
        HAL_GPIO_Init(g_key_hw[i].port, &gpio_init);
    }
}

key_state_t key_read(key_id_t id)
{
    GPIO_PinState level;

    if (id >= KEY_NUM)
    {
        return KEY_RELEASED;
    }

    level = HAL_GPIO_ReadPin(g_key_hw[id].port, g_key_hw[id].pin);

    if (g_key_hw[id].active_low)
    {
        return (level == GPIO_PIN_RESET) ? KEY_PRESSED : KEY_RELEASED;
    }

    return (level == GPIO_PIN_SET) ? KEY_PRESSED : KEY_RELEASED;
}

key_id_t key_scan(bool continuous)
{
    static bool key_up = true;
    key_id_t id = KEY_NONE;
    bool any;

    if (continuous)
    {
        key_up = true;
    }

    if (key_up)
    {
        any = (key_read(KEY0) == KEY_PRESSED) || (key_read(KEY1) == KEY_PRESSED) ||
              (key_read(KEY2) == KEY_PRESSED) || (key_read(KEY_WKUP) == KEY_PRESSED);
        if (any)
        {
            delay_ms(10);
            key_up = false;

            if (key_read(KEY0) == KEY_PRESSED)
            {
                id = KEY0;
            }
            else if (key_read(KEY1) == KEY_PRESSED)
            {
                id = KEY1;
            }
            else if (key_read(KEY2) == KEY_PRESSED)
            {
                id = KEY2;
            }
            else if (key_read(KEY_WKUP) == KEY_PRESSED)
            {
                id = KEY_WKUP;
            }
        }
    }
    else if ((key_read(KEY0) == KEY_RELEASED) && (key_read(KEY1) == KEY_RELEASED) &&
             (key_read(KEY2) == KEY_RELEASED) && (key_read(KEY_WKUP) == KEY_RELEASED))
    {
        key_up = true;
    }

    return id;
}
