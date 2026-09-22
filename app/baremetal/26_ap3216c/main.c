/**
 * @file    main.c
 * @brief   26_ap3216c: AP3216C ambient light / proximity sensor test. The raw
 *          IR, PS and ALS channels are shown on the RGB panel and USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U
#define SAMPLE_PERIOD   120U
#define VALUE_X         (TEXT_X + 48U)

int main(void)
{
    char line[48];
    uint16_t ir;
    uint16_t ps;
    uint16_t als;

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "AP3216C TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    if (ap3216c_init() != 0U)
    {
        lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "AP3216C Check Failed!", RED);
        printf("AP3216C check failed\r\n");
    }
    else
    {
        lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "AP3216C Ready!", BLUE);
        printf("AP3216C ready\r\n");
    }

    lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, " IR:", RED);
    lcd_show_string(TEXT_X, 150U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, " PS:", RED);
    lcd_show_string(TEXT_X, 170U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ALS:", RED);

    printf("26_ap3216c ready\r\n");

    for (;;)
    {
        ap3216c_read_data(&ir, &ps, &als);

        lcd_show_num(VALUE_X, 130U, ir, 5U, LCD_FONT_SIZE_16, BLUE);
        lcd_show_num(VALUE_X, 150U, ps, 5U, LCD_FONT_SIZE_16, BLUE);
        lcd_show_num(VALUE_X, 170U, als, 5U, LCD_FONT_SIZE_16, BLUE);

        sprintf(line, "IR:%u PS:%u ALS:%u", (unsigned)ir, (unsigned)ps, (unsigned)als);
        printf("%s\r\n", line);

        led_toggle(LED0);
        delay_ms(SAMPLE_PERIOD);
    }
}
