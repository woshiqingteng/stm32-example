/**
 * @file    main.c
 * @brief   22_1_dac: DAC1 channel 1 (PA4) software-triggered ramp output. The
 *          programmed code and its equivalent voltage are shown on the LCD.
 */

#include <stdio.h>
#include "bsp.h"

#define DAC_RAMP_STEP     64U
#define DAC_RAMP_DELAY_MS 5U
#define DAC_TEXT_X        30U
#define DAC_TEXT_WIDTH    240U
#define DAC_MV_FULL       3300U

static void dac_show(uint16_t code)
{
    char     buf[40];
    uint32_t millivolt = ((uint32_t)code * DAC_MV_FULL) / DAC_FULL_SCALE;

    sprintf(buf, "DAC: %4u  %lu.%03luV", (unsigned)code,
            (unsigned long)(millivolt / 1000U),
            (unsigned long)(millivolt % 1000U));
    lcd_show_string(DAC_TEXT_X, 150U, DAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, buf, BLUE);
    printf("%s\r\n", buf);
}

int main(void)
{
    uint16_t code;

    bsp_init();
    sdram_init();
    lcd_init();
    dac_init();

    lcd_clear(WHITE);
    lcd_show_string(DAC_TEXT_X, 50U, DAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(DAC_TEXT_X, 70U, DAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DAC OUTPUT TEST", RED);
    lcd_show_string(DAC_TEXT_X, 90U, DAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(DAC_TEXT_X, 120U, DAC_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Ramp on PA4 (DAC1_OUT1)", BLACK);

    printf("22_1_dac ready\r\n");

    for (;;)
    {
        for (code = 0U; code <= DAC_FULL_SCALE; code += DAC_RAMP_STEP)
        {
            dac_set(DAC_CH1, code);
            dac_show(code);
            led_toggle(LED0);
            delay_ms(DAC_RAMP_DELAY_MS);
        }

        for (code = DAC_FULL_SCALE; code >= DAC_RAMP_STEP; code -= DAC_RAMP_STEP)
        {
            dac_set(DAC_CH1, code);
            dac_show(code);
            led_toggle(LED0);
            delay_ms(DAC_RAMP_DELAY_MS);
        }

        dac_set(DAC_CH1, 0U);
        dac_show(0U);
    }
}
