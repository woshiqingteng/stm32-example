/**
 * @file    exti.c
 * @brief   On-board key external-interrupt driver (function-pointer dispatch).
 */

#include "stm32f4xx_hal.h"
#include "exti.h"

typedef struct
{
    key_id_t      id;
    GPIO_TypeDef *port;
    uint16_t      pin;
    IRQn_Type     irqn;
    uint32_t      mode;
    uint32_t      pull;
} exti_hw_t;

static const exti_hw_t g_exti_hw[KEY_NUM] =
{
    { KEY0,     GPIOH, GPIO_PIN_3,  EXTI3_IRQn,     GPIO_MODE_IT_FALLING, GPIO_PULLUP   },
    { KEY1,     GPIOH, GPIO_PIN_2,  EXTI2_IRQn,     GPIO_MODE_IT_FALLING, GPIO_PULLUP   },
    { KEY2,     GPIOC, GPIO_PIN_13, EXTI15_10_IRQn, GPIO_MODE_IT_FALLING, GPIO_PULLUP   },
    { KEY_WKUP, GPIOA, GPIO_PIN_0,  EXTI0_IRQn,     GPIO_MODE_IT_RISING,  GPIO_PULLDOWN },
};

static exti_cb_t g_exti_cb[KEY_NUM];

static void exti_dispatch(uint16_t pin)
{
    uint32_t i;

    for (i = 0; i < KEY_NUM; i++)
    {
        if (g_exti_hw[i].pin == pin)
        {
            if (g_exti_cb[i] != 0)
            {
                g_exti_cb[i](g_exti_hw[i].id);
            }
            break;
        }
    }
}

void EXTI0_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
    exti_dispatch(GPIO_PIN_0);
}

void EXTI2_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
    exti_dispatch(GPIO_PIN_2);
}

void EXTI3_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
    exti_dispatch(GPIO_PIN_3);
}

void EXTI15_10_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
    exti_dispatch(GPIO_PIN_13);
}

void exti_register(key_id_t id, exti_cb_t cb)
{
    if (id < KEY_NUM)
    {
        g_exti_cb[id] = cb;
    }
}

void exti_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint32_t i;

    key_init();

    for (i = 0; i < KEY_NUM; i++)
    {
        gpio_init.Pin  = g_exti_hw[i].pin;
        gpio_init.Mode = g_exti_hw[i].mode;
        gpio_init.Pull = g_exti_hw[i].pull;
        HAL_GPIO_Init(g_exti_hw[i].port, &gpio_init);

        HAL_NVIC_SetPriority(g_exti_hw[i].irqn, (uint32_t)i, 2);
        HAL_NVIC_EnableIRQ(g_exti_hw[i].irqn);
    }
}
