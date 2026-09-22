/**
 * @file    main.c
 * @brief   22_2_dac_tri: DAC1 channel 1 triangle wave generated from a buffer
 *          by TIM6 TRGO + DMA1 (circular).
 */

#include <stdio.h>
#include "bsp.h"

#define TRI_TIMER_ARR 899U
#define TRI_TIMER_PSC 0U

#define TRI_TEXT_X     30U
#define TRI_TEXT_WIDTH 240U

int main(void)
{
    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TRI_TEXT_X, 50U, TRI_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TRI_TEXT_X, 70U, TRI_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DAC TRIANGLE WAVE", RED);
    lcd_show_string(TRI_TEXT_X, 90U, TRI_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(TRI_TEXT_X, 120U, TRI_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "OUT1 PA4  TIM6+DMA", BLACK);
    lcd_show_string(TRI_TEXT_X, 150U, TRI_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "approx. 1 kHz", BLUE);

    dac_triangle_init(TRI_TIMER_ARR, TRI_TIMER_PSC);
    dac_triangle_start();

    printf("22_2_dac_tri ready, ~1kHz triangle on PA4\r\n");

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
