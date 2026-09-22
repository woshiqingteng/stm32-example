/**
 * @file    main.c
 * @brief   21_adc_temp: internal temperature sensor read (ADC1 channel 18).
 *          Temperature is reported as degrees * 100 and shown on the RGB panel.
 */

#include <stdio.h>
#include "bsp.h"

#define TEMP_SAMPLE_PERIOD_MS 500U
#define TEMP_TEXT_X           30U
#define TEMP_TEXT_WIDTH       240U

int main(void)
{
    char buf[32];

    bsp_init();
    sdram_init();
    lcd_init();
    adc_temp_init();

    lcd_clear(WHITE);
    lcd_show_string(TEMP_TEXT_X, 50U, TEMP_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEMP_TEXT_X, 70U, TEMP_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Temperature TEST", RED);
    lcd_show_string(TEMP_TEXT_X, 90U, TEMP_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    printf("21_adc_temp ready\r\n");

    for (;;)
    {
        int16_t temp = adc_get_temperature();
        int16_t temp_abs = (temp < 0) ? (int16_t)(-temp) : temp;

        sprintf(buf, "TEMP: %s%d.%02dC", (temp < 0) ? "-" : "",
                (int)(temp_abs / 100), (int)(temp_abs % 100));
        lcd_show_string(TEMP_TEXT_X, 120U, TEMP_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, buf, BLUE);
        printf("%s\r\n", buf);

        led_toggle(LED0);
        delay_ms(TEMP_SAMPLE_PERIOD_MS);
    }
}
