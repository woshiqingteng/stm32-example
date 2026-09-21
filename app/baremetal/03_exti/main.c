/**
 * @file    main.c
 * @brief   03_exti: toggle LEDs from key external interrupts.
 */

#include "bsp.h"

#define KEY_EXTI_DEBOUNCE_MS 20U
#define IDLE_DELAY_MS        1000U

static void on_key0(key_id_t id)
{
    (void)id;
    delay_ms(KEY_EXTI_DEBOUNCE_MS);
    led_toggle(LED0);
    led_toggle(LED1);
}

static void on_key1(key_id_t id)
{
    (void)id;
    delay_ms(KEY_EXTI_DEBOUNCE_MS);
    led_toggle(LED1);
}

static void on_key2(key_id_t id)
{
    (void)id;
    delay_ms(KEY_EXTI_DEBOUNCE_MS);
    led_toggle(LED0);
}

static void on_key_wkup(key_id_t id)
{
    (void)id;
    delay_ms(KEY_EXTI_DEBOUNCE_MS);
    led_toggle(LED1);
}

int main(void)
{
    bsp_init();
    led_on(LED0);

    exti_init();
    exti_register(KEY0, on_key0);
    exti_register(KEY1, on_key1);
    exti_register(KEY2, on_key2);
    exti_register(KEY_WKUP, on_key_wkup);

    for (;;)
    {
        delay_ms(IDLE_DELAY_MS);
    }
}
