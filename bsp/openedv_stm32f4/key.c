/**
 * @file    key.c
 * @brief   On-board key driver.
 */

#include "stm32f4xx_hal.h"
#include "key.h"
#include "delay.h"

#define KEY_DEBOUNCE_MS     10U

#define KEY0_ACTIVE_LOW     1U
#define KEY1_ACTIVE_LOW     1U
#define KEY2_ACTIVE_LOW     1U
#define KEY_WKUP_ACTIVE_LOW 0U

/* Scan latch: WAIT_PRESS arms reporting, WAIT_RELEASE prevents repeats. */
typedef enum
{
    KEY_SCAN_WAIT_PRESS = 0,
    KEY_SCAN_WAIT_RELEASE,
} key_scan_state_t;

static void key_gpio_config(GPIO_TypeDef *port, uint16_t pin, uint32_t pull)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin  = pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = pull;
    HAL_GPIO_Init(port, &gpio_init);
}

void key_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    key_gpio_config(KEY0_GPIO_PORT, KEY0_GPIO_PIN, GPIO_PULLUP);
    key_gpio_config(KEY1_GPIO_PORT, KEY1_GPIO_PIN, GPIO_PULLUP);
    key_gpio_config(KEY2_GPIO_PORT, KEY2_GPIO_PIN, GPIO_PULLUP);
    key_gpio_config(KEY_WKUP_GPIO_PORT, KEY_WKUP_GPIO_PIN, GPIO_PULLDOWN);
}

key_state_t key_read(key_id_t id)
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          active_low;
    GPIO_PinState level;

    switch (id)
    {
        case KEY0:
            port = KEY0_GPIO_PORT;
            pin = KEY0_GPIO_PIN;
            active_low = KEY0_ACTIVE_LOW;
            break;
        case KEY1:
            port = KEY1_GPIO_PORT;
            pin = KEY1_GPIO_PIN;
            active_low = KEY1_ACTIVE_LOW;
            break;
        case KEY2:
            port = KEY2_GPIO_PORT;
            pin = KEY2_GPIO_PIN;
            active_low = KEY2_ACTIVE_LOW;
            break;
        case KEY_WKUP:
            port = KEY_WKUP_GPIO_PORT;
            pin = KEY_WKUP_GPIO_PIN;
            active_low = KEY_WKUP_ACTIVE_LOW;
            break;
        default:
            return KEY_RELEASED;
    }

    level = HAL_GPIO_ReadPin(port, pin);

    if (active_low)
    {
        return (level == GPIO_PIN_RESET) ? KEY_PRESSED : KEY_RELEASED;
    }

    return (level == GPIO_PIN_SET) ? KEY_PRESSED : KEY_RELEASED;
}

key_id_t key_scan(bool continuous)
{
    static key_scan_state_t scan_state = KEY_SCAN_WAIT_PRESS;
    key_id_t id = KEY_NONE;
    bool any;

    if (continuous)
    {
        scan_state = KEY_SCAN_WAIT_PRESS;
    }

    if (scan_state == KEY_SCAN_WAIT_PRESS)
    {
        any = (key_read(KEY0) == KEY_PRESSED) || (key_read(KEY1) == KEY_PRESSED) ||
              (key_read(KEY2) == KEY_PRESSED) || (key_read(KEY_WKUP) == KEY_PRESSED);
        if (any)
        {
            delay_ms(KEY_DEBOUNCE_MS);
            scan_state = KEY_SCAN_WAIT_RELEASE;

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
        scan_state = KEY_SCAN_WAIT_PRESS;
    }

    return id;
}
