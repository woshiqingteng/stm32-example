/**
 * @file    main.c
 * @brief   09_1_atim_npwm: TIM8_CH1 (PC6) emits a given number of PWM pulses.
 *          PB0 is set to input so PC6 can be jumpered to LED1 (PB0).
 */

#include "stm32f4xx_hal.h"
#include "bsp.h"

#define ATIM_NPWM_ARR     10000U
#define ATIM_NPWM_PSC     9000U
#define ATIM_NPWM_PULSES  5U
#define ATIM_NPWM_LOOP_MS 500U

int main(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    bsp_init();

    /* Free PB0 (LED1) and use it as the pulse observation input. */
    gpio_init.Pin  = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    atim_timx_npwm_chy_init(ATIM_NPWM_ARR - 1U, ATIM_NPWM_PSC - 1U);
    atim_timx_npwm_chy_set(ATIM_NPWM_PULSES);

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            atim_timx_npwm_chy_set(ATIM_NPWM_PULSES);
        }

        led_toggle(LED0);
        delay_ms(ATIM_NPWM_LOOP_MS);
    }
}
