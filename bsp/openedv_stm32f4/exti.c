/**
 * @file    exti.c
 * @brief   On-board key external-interrupt driver (function-pointer dispatch).
 */

#include "stm32f4xx_hal.h"
#include "exti.h"
#include "key.h"

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

#define EXTI_DEBOUNCE_MS   20U

static exti_cb_t g_exti_cb[KEY_NUM];

static volatile key_id_t g_pending_id   = KEY_NONE;
static volatile uint32_t g_pending_tick = 0U;

/* ISR side: latch the edge and (re)start the debounce window. Non-blocking. */
static void exti_latch(key_id_t id)
{
    g_pending_tick = HAL_GetTick();
    g_pending_id   = id;
}

void EXTI0_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(key_pin(KEY_WKUP));
    exti_latch(KEY_WKUP);
}

void EXTI2_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(key_pin(KEY1));
    exti_latch(KEY1);
}

void EXTI3_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(key_pin(KEY0));
    exti_latch(KEY0);
}

void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(key_pin(KEY2)) != 0U)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(key_pin(KEY2));
        exti_latch(KEY2);
    }
}

void exti_register(key_id_t id, exti_cb_t cb)
{
    if (id < KEY_NUM)
    {
        g_exti_cb[id] = cb;
    }
}

/* Main-loop side: report once the edge has settled and the key is still held. */
void exti_poll(void)
{
    key_id_t id = g_pending_id;

    if (id == KEY_NONE)
    {
        return;
    }
    if ((HAL_GetTick() - g_pending_tick) < EXTI_DEBOUNCE_MS)
    {
        return;
    }

    g_pending_id = KEY_NONE;
    if (g_exti_cb[id] != 0 && key_is_pressed(id))
    {
        g_exti_cb[id](id);
    }
}

static void exti_config(GPIO_TypeDef *port, uint16_t pin, uint32_t mode, uint32_t pull, IRQn_Type irqn,
                        uint32_t preempt)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin  = pin;
    gpio_init.Mode = mode;
    gpio_init.Pull = pull;
    HAL_GPIO_Init(port, &gpio_init);

    HAL_NVIC_SetPriority(irqn, preempt, 2U);
    HAL_NVIC_EnableIRQ(irqn);
}

void exti_init(void)
{
    key_init();

    exti_config(key_port(KEY0), key_pin(KEY0), EXTI_KEY0_MODE, EXTI_KEY0_PULL, EXTI_KEY0_IRQn,
                0U);
    exti_config(key_port(KEY1), key_pin(KEY1), EXTI_KEY1_MODE, EXTI_KEY1_PULL, EXTI_KEY1_IRQn,
                1U);
    exti_config(key_port(KEY2), key_pin(KEY2), EXTI_KEY2_MODE, EXTI_KEY2_PULL, EXTI_KEY2_IRQn,
                2U);
    exti_config(key_port(KEY_WKUP), key_pin(KEY_WKUP), EXTI_KEY_WKUP_MODE, EXTI_KEY_WKUP_PULL,
                EXTI_KEY_WKUP_IRQn, 3U);
}
