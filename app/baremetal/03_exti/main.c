/**
 * @file    main.c
 * @brief   03_exti: toggle LEDs from key external interrupts.
 */

#include <stdio.h>
#include "bsp.h"
#define POLL_DELAY_MS 1U

static void on_key0(key_id_t id)
{
    (void)id;
    led_toggle(LED0);
    led_toggle(LED1);
}

static void on_key1(key_id_t id)
{
    (void)id;
    led_toggle(LED1);
}

static void on_key2(key_id_t id)
{
    (void)id;
    led_toggle(LED0);
}

static void on_key_wkup(key_id_t id)
{
    (void)id;
    led_toggle(LED1);

    if (led_is_on(LED1))
    {
        led_off(LED0);
    }
    else
    {
        led_on(LED0);
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    led_on(LED0);

    /* Edge -> EXTIx_IRQHandler (latch only) -> main-loop exti_poll()
       (20 ms debounce + level re-check) -> matching on_keyN() action. */
    exti_init();
    exti_register(KEY0, &on_key0);
    exti_register(KEY1, &on_key1);
    exti_register(KEY2, &on_key2);
    exti_register(KEY_WKUP, &on_key_wkup);

    for (;;)
    {
        exti_poll();
        delay_ms(POLL_DELAY_MS);
    }
}
