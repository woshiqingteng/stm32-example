/**
 * @file    main.c
 * @brief   03_exti: toggle LEDs from key external interrupts.
 */

#include "bsp.h"

static void on_key(key_id_t id)
{
    delay_ms(20);

    switch (id)
    {
        case KEY0:
            led_toggle(LED0);
            led_toggle(LED1);
            break;
        case KEY1:
            led_toggle(LED1);
            break;
        case KEY2:
            led_toggle(LED0);
            break;
        case KEY_WKUP:
            led_toggle(LED1);
            break;
        default:
            break;
    }
}

int main(void)
{
    bsp_init();
    led_on(LED0);

    exti_init();
    exti_register(KEY0, on_key);
    exti_register(KEY1, on_key);
    exti_register(KEY2, on_key);
    exti_register(KEY_WKUP, on_key);

    for (;;)
    {
        delay_ms(1000);
    }
}
