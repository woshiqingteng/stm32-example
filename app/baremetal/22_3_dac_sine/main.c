/**
 * @file    main.c
 * @brief   22_3_dac_sine: DAC1 channel 1 sine wave generated from a 100-point
 *          table by TIM7 TRGO + DMA1 (circular).
 */

#include <stdio.h>
#include "bsp.h"

#define SIN_TIMER_ARR 9U
#define SIN_TIMER_PSC 29U

#define SIN_TEXT_X     30U
#define SIN_TEXT_WIDTH 240U

int main(void)
{
    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(SIN_TEXT_X, 50U, SIN_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(SIN_TEXT_X, 70U, SIN_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DAC SINE WAVE", RED);
    lcd_show_string(SIN_TEXT_X, 90U, SIN_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(SIN_TEXT_X, 120U, SIN_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "OUT1 PA4  TIM7+DMA", BLACK);
    lcd_show_string(SIN_TEXT_X, 150U, SIN_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "approx. 3 kHz", BLUE);

    dac_sine_init(SIN_TIMER_ARR, SIN_TIMER_PSC);
    dac_sine_start();

    printf("22_3_dac_sine ready, ~3kHz sine on PA4\r\n");

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
