/**
 * @file    main.c
 * @brief   09_1_atim_npwm: TIM8_CH1 (PC6) emits a given number of PWM pulses.
 *          PB0 is set to input so PC6 can be jumpered to LED1 (PB0).
 */

#include "stm32f4xx_hal.h"
#include "bsp.h"

int main(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    bsp_init();

    /* Free PB0 (LED1) and use it as the pulse observation input. */
    gpio_init.Pin  = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    atim_timx_npwm_chy_init(10000 - 1, 9000 - 1);
    atim_timx_npwm_chy_set(5);

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            atim_timx_npwm_chy_set(5);
        }

        led_toggle(LED0);
        delay_ms(500);
    }
}
