/**
 * @file    main.c
 * @brief   25_io_extend: PCF8574 8-bit IO expander test. Alternating patterns
 *          are written and read back; the expander INT line and the EX_IO input
 *          are also polled. Results are shown on the RGB panel and USART1.
 */

#include <stdbool.h>
#include <stdio.h>
#include "bsp.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U
#define PATTERN_PERIOD  500U

static const uint8_t g_patterns[] = { 0xAAU, 0x55U };

int main(void)
{
    char line[48];
    uint8_t idx = 0U;
    uint8_t status;
    bool ack = false;

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "PCF8574 IO EXTEND TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    if (pcf8574_init() != 0U)
    {
        lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "PCF8574 Check Failed!", RED);
        printf("PCF8574 check failed\r\n");
    }
    else
    {
        lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "PCF8574 Ready!", BLUE);
        printf("PCF8574 ready\r\n");
    }

    printf("25_io_extend ready\r\n");

    for (;;)
    {
        uint8_t write_val = g_patterns[idx];
        uint8_t read_val;

        pcf8574_write_byte(write_val);
        read_val = pcf8574_read_byte();

        ack = (read_val == write_val);

        sprintf(line, "Write:0x%02X Read:0x%02X", write_val, read_val);
        lcd_show_string(TEXT_X, 120U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line,
                        ack ? BLUE : RED);
        printf("%s\r\n", line);

        status = (pcf8574_int_asserted()) ? 1U : 0U;
        sprintf(line, "INT:%u EX_IO:%u", status, (unsigned)pcf8574_read_bit(PCF8574_EX_IO));
        lcd_show_string(TEXT_X, 140U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
        printf("%s\r\n", line);

        idx ^= 1U;
        led_toggle(LED0);
        delay_ms(PATTERN_PERIOD);
    }
}
