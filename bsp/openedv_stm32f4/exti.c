/**
 * @file    exti.c
 * @brief   On-board key external-interrupt driver (function-pointer dispatch).
 */

#include "stm32f4xx_hal.h"
#include "exti.h"
#include "key.h"

#define EXTI_IRQ_PREEMPT  3U
#define EXTI_IRQ_SUB      2U

#define EXTI_KEY0_IRQn    EXTI3_IRQn
#define EXTI_KEY0_MODE    GPIO_MODE_IT_FALLING
#define EXTI_KEY0_PULL    GPIO_PULLUP

#define EXTI_KEY1_IRQn    EXTI2_IRQn
#define EXTI_KEY1_MODE    GPIO_MODE_IT_FALLING
#define EXTI_KEY1_PULL    GPIO_PULLUP

#define EXTI_KEY2_IRQn    EXTI15_10_IRQn
#define EXTI_KEY2_MODE    GPIO_MODE_IT_FALLING
#define EXTI_KEY2_PULL    GPIO_PULLUP

#define EXTI_KEY_WKUP_IRQn EXTI0_IRQn
#define EXTI_KEY_WKUP_MODE GPIO_MODE_IT_RISING
#define EXTI_KEY_WKUP_PULL GPIO_PULLDOWN

static exti_cb_t g_exti_cb[KEY_NUM];

static void exti_dispatch(uint16_t pin)
{
    switch (pin)
    {
        case KEY0_GPIO_PIN:
            if (g_exti_cb[KEY0] != 0)
            {
                g_exti_cb[KEY0](KEY0);
            }
            break;
        case KEY1_GPIO_PIN:
            if (g_exti_cb[KEY1] != 0)
            {
                g_exti_cb[KEY1](KEY1);
            }
            break;
        case KEY2_GPIO_PIN:
            if (g_exti_cb[KEY2] != 0)
            {
                g_exti_cb[KEY2](KEY2);
            }
            break;
        case KEY_WKUP_GPIO_PIN:
            if (g_exti_cb[KEY_WKUP] != 0)
            {
                g_exti_cb[KEY_WKUP](KEY_WKUP);
            }
            break;
        default:
            break;
    }
}

void EXTI0_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(KEY_WKUP_GPIO_PIN);
    exti_dispatch(KEY_WKUP_GPIO_PIN);
}

void EXTI2_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(KEY1_GPIO_PIN);
    exti_dispatch(KEY1_GPIO_PIN);
}

void EXTI3_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(KEY0_GPIO_PIN);
    exti_dispatch(KEY0_GPIO_PIN);
}

void EXTI15_10_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(KEY2_GPIO_PIN);
    exti_dispatch(KEY2_GPIO_PIN);
}

void exti_register(key_id_t id, exti_cb_t cb)
{
    if (id < KEY_NUM)
    {
        g_exti_cb[id] = cb;
    }
}

static void exti_config(GPIO_TypeDef *port, uint16_t pin, uint32_t mode, uint32_t pull, IRQn_Type irqn)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin  = pin;
    gpio_init.Mode = mode;
    gpio_init.Pull = pull;
    HAL_GPIO_Init(port, &gpio_init);

    HAL_NVIC_SetPriority(irqn, EXTI_IRQ_PREEMPT, EXTI_IRQ_SUB);
    HAL_NVIC_EnableIRQ(irqn);
}

void exti_init(void)
{
    key_init();

    exti_config(KEY0_GPIO_PORT, KEY0_GPIO_PIN, EXTI_KEY0_MODE, EXTI_KEY0_PULL, EXTI_KEY0_IRQn);
    exti_config(KEY1_GPIO_PORT, KEY1_GPIO_PIN, EXTI_KEY1_MODE, EXTI_KEY1_PULL, EXTI_KEY1_IRQn);
    exti_config(KEY2_GPIO_PORT, KEY2_GPIO_PIN, EXTI_KEY2_MODE, EXTI_KEY2_PULL, EXTI_KEY2_IRQn);
    exti_config(KEY_WKUP_GPIO_PORT, KEY_WKUP_GPIO_PIN, EXTI_KEY_WKUP_MODE, EXTI_KEY_WKUP_PULL,
                EXTI_KEY_WKUP_IRQn);
}
