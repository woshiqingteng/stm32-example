/**
 * @file    main.c
 * @brief   23_pwm_dac: filtered PWM on TIM9_CH2 (PA3) used as a DAC. The duty
 *          (CCR) and its equivalent output voltage are shown on the LCD.
 */

#include <stdio.h>
#include "bsp.h"

#define PWMDAC_ARR       255U
#define PWMDAC_PSC       1U
#define PWMDAC_STEP_MV   100U
#define PWMDAC_DELAY_MS  20U

#define PWMDAC_TEXT_X     30U
#define PWMDAC_TEXT_WIDTH 240U

static void pwmdac_show(void)
{
    char     buf[40];
    uint16_t vol = (uint16_t)(((uint32_t)pwmdac_get_code() * PWMDAC_VREF_MV) /
                              (PWMDAC_ARR + 1U));

    sprintf(buf, "PWM: %3lu  %u.%03luV",
            (unsigned long)pwmdac_get_code(),
            (unsigned)(vol / 1000U), (unsigned long)(vol % 1000U));
    lcd_show_string(PWMDAC_TEXT_X, 150U, PWMDAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, buf, BLUE);
    printf("%s\r\n", buf);
}

int main(void)
{
    uint16_t vol;

    bsp_init();
    sdram_init();
    lcd_init();
    pwmdac_init(PWMDAC_ARR, PWMDAC_PSC);

    lcd_clear(WHITE);
    lcd_show_string(PWMDAC_TEXT_X, 50U, PWMDAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(PWMDAC_TEXT_X, 70U, PWMDAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "PWM DAC TEST", RED);
    lcd_show_string(PWMDAC_TEXT_X, 90U, PWMDAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(PWMDAC_TEXT_X, 120U, PWMDAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "TIM9_CH2 on PA3", BLACK);

    printf("23_pwm_dac ready\r\n");

    for (;;)
    {
        for (vol = 0U; vol <= PWMDAC_VREF_MV; vol += PWMDAC_STEP_MV)
        {
            pwmdac_set(vol);
            pwmdac_show();
            led_toggle(LED0);
            delay_ms(PWMDAC_DELAY_MS);
        }

        for (vol = PWMDAC_VREF_MV; vol >= PWMDAC_STEP_MV; vol -= PWMDAC_STEP_MV)
        {
            pwmdac_set(vol);
            pwmdac_show();
            led_toggle(LED0);
            delay_ms(PWMDAC_DELAY_MS);
        }
    }
}
