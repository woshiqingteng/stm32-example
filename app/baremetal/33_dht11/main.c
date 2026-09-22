/**
 * @file    main.c
 * @brief   33_dht11: DHT11 temperature / humidity test. A measurement is read
 *          periodically and shown on the RGB panel and USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define DHT11_PERIOD_MS 1000U

#define TEXT_X          30U
#define TEXT_WIDTH      300U

int main(void)
{
    uint8_t temperature = 0U;
    uint8_t humidity    = 0U;
    char    line[48];

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DHT11 TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    if (dht11_init() != 0U)
    {
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DHT11 not found!", RED);
        printf("DHT11 not found!\r\n");

        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "DHT11 Ready!", BLUE);
    printf("33_dht11 ready\r\n");

    for (;;)
    {
        if (dht11_read_data(&temperature, &humidity) != 0U)
        {
            lcd_show_string(TEXT_X, 140U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Read failed!", RED);
            printf("DHT11 read failed\r\n");
        }
        else
        {
            sprintf(line, "Temp: %u C  Humi: %u %%", temperature, humidity);
            lcd_show_string(TEXT_X, 140U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
            printf("%s\r\n", line);
        }

        led_toggle(LED0);
        delay_ms(DHT11_PERIOD_MS);
    }
}
