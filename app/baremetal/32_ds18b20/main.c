/**
 * @file    main.c
 * @brief   32_ds18b20: DS18B20 1-Wire temperature test. The temperature is read
 *          periodically and shown on the RGB panel and USART1 (tenths of a
 *          degree resolution).
 */

#include <stdio.h>
#include "bsp.h"

#define TEMP_PERIOD_MS  500U

#define TEXT_X          30U
#define TEXT_WIDTH      300U

int main(void)
{
    int16_t temperature;
    int16_t shown;
    char    line[48];

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DS18B20 TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    if (ds18b20_init() != 0U)
    {
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DS18B20 not found!", RED);
        printf("DS18B20 not found!\r\n");

        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DS18B20 Ready!", BLUE);
    printf("32_ds18b20 ready\r\n");

    for (;;)
    {
        temperature = ds18b20_get_temperature();
        shown = (temperature < 0) ? (int16_t)(-temperature) : temperature;

        sprintf(line, "Temp: %s%d.%d C", (temperature < 0) ? "-" : "",
                shown / 10, shown % 10);
        lcd_show_string(TEXT_X, 140U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
        printf("%s\r\n", line);

        led_toggle(LED0);
        delay_ms(TEMP_PERIOD_MS);
    }
}
