/**
 * @file    key.c
 * @brief   On-board key driver.
 */

#include "stm32f4xx_hal.h"
#include "key.h"
#include "delay.h"

#define KEY_DEBOUNCE_MS     10U

/* On-board keys: KEY0 = PH3, KEY1 = PH2, KEY2 = PC13, WK_UP = PA0. */
#define KEY0_GPIO_PORT      GPIOH
#define KEY0_GPIO_PIN       GPIO_PIN_3
#define KEY0_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOH_CLK_ENABLE(); } while (0)

#define KEY1_GPIO_PORT      GPIOH
#define KEY1_GPIO_PIN       GPIO_PIN_2
#define KEY1_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOH_CLK_ENABLE(); } while (0)

#define KEY2_GPIO_PORT      GPIOC
#define KEY2_GPIO_PIN       GPIO_PIN_13
#define KEY2_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

#define KEY_WKUP_GPIO_PORT  GPIOA
#define KEY_WKUP_GPIO_PIN   GPIO_PIN_0
#define KEY_WKUP_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

#define KEY0_PRESSED_LEVEL     GPIO_PIN_RESET
#define KEY1_PRESSED_LEVEL     GPIO_PIN_RESET
#define KEY2_PRESSED_LEVEL     GPIO_PIN_RESET
#define KEY_WKUP_PRESSED_LEVEL GPIO_PIN_SET

/* Scan latch: WAIT_PRESS arms reporting, WAIT_RELEASE prevents repeats. */
typedef enum
{
    KEY_SCAN_WAIT_PRESS = 0,
    KEY_SCAN_WAIT_RELEASE = 1,
} key_scan_state_t;

typedef enum
{
    KEY_RELEASED = 0,
    KEY_PRESSED = 1,
} key_state_t;

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
    KEY0_GPIO_CLK_ENABLE();
    KEY1_GPIO_CLK_ENABLE();
    KEY2_GPIO_CLK_ENABLE();
    KEY_WKUP_GPIO_CLK_ENABLE();

    key_gpio_config(KEY0_GPIO_PORT, KEY0_GPIO_PIN, GPIO_PULLUP);
    key_gpio_config(KEY1_GPIO_PORT, KEY1_GPIO_PIN, GPIO_PULLUP);
    key_gpio_config(KEY2_GPIO_PORT, KEY2_GPIO_PIN, GPIO_PULLUP);
    key_gpio_config(KEY_WKUP_GPIO_PORT, KEY_WKUP_GPIO_PIN, GPIO_PULLDOWN);
}

GPIO_TypeDef *key_port(key_id_t id)
{
    switch (id)
    {
        case KEY0:     return KEY0_GPIO_PORT;
        case KEY1:     return KEY1_GPIO_PORT;
        case KEY2:     return KEY2_GPIO_PORT;
        case KEY_WKUP: return KEY_WKUP_GPIO_PORT;
        default:       return 0;
    }
}

uint16_t key_pin(key_id_t id)
{
    switch (id)
    {
        case KEY0:     return KEY0_GPIO_PIN;
        case KEY1:     return KEY1_GPIO_PIN;
        case KEY2:     return KEY2_GPIO_PIN;
        case KEY_WKUP: return KEY_WKUP_GPIO_PIN;
        default:       return 0U;
    }
}

static key_state_t key_read(key_id_t id)
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    GPIO_PinState pressed_level;

    switch (id)
    {
        case KEY0:
            port = KEY0_GPIO_PORT;
            pin = KEY0_GPIO_PIN;
            pressed_level = KEY0_PRESSED_LEVEL;
            break;
        case KEY1:
            port = KEY1_GPIO_PORT;
            pin = KEY1_GPIO_PIN;
            pressed_level = KEY1_PRESSED_LEVEL;
            break;
        case KEY2:
            port = KEY2_GPIO_PORT;
            pin = KEY2_GPIO_PIN;
            pressed_level = KEY2_PRESSED_LEVEL;
            break;
        case KEY_WKUP:
            port = KEY_WKUP_GPIO_PORT;
            pin = KEY_WKUP_GPIO_PIN;
            pressed_level = KEY_WKUP_PRESSED_LEVEL;
            break;
        default:
            return KEY_RELEASED;
    }

    return (HAL_GPIO_ReadPin(port, pin) == pressed_level) ? KEY_PRESSED : KEY_RELEASED;
}

bool key_is_pressed(key_id_t id)
{
    return key_read(id) == KEY_PRESSED;
}

key_id_t key_scan(bool continuous)
{
    /* static: persists across calls (unlike id); latches a press until release. */
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

            if (key_read(KEY_WKUP) == KEY_PRESSED)
            {
                id = KEY_WKUP;
            }
            else if (key_read(KEY2) == KEY_PRESSED)
            {
                id = KEY2;
            }
            else if (key_read(KEY1) == KEY_PRESSED)
            {
                id = KEY1;
            }
            else if (key_read(KEY0) == KEY_PRESSED)
            {
                id = KEY0;
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
