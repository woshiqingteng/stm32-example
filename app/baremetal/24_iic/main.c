/**
 * @file    main.c
 * @brief   24_iic: AT24C02 EEPROM write/read test over the software IIC bus.
 *          A byte pattern and a string are written, read back, compared and
 *          reported on the RGB panel and USART1.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "bsp.h"

#define EEPROM_TEST_ADDR   0U
#define EEPROM_BYTE_COUNT  16U
#define EEPROM_STR_LEN     14U
#define EEPROM_STR_ADDR    (EEPROM_TEST_ADDR + EEPROM_BYTE_COUNT)

#define TEXT_X             30U
#define TEXT_WIDTH         300U
#define LINE_HEIGHT        20U

static const uint8_t g_pattern[EEPROM_BYTE_COUNT] =
{
    0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U,
    0x88U, 0x99U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU
};

static const char g_text[EEPROM_STR_LEN] = "STM32 IIC TEST";

static uint8_t g_readback[EEPROM_BYTE_COUNT];

int main(void)
{
    char line[48];
    uint8_t i;
    bool byte_ok = true;
    bool text_ok;
    char text_read[EEPROM_STR_LEN + 1U];

    bsp_init();
    sdram_init();
    lcd_init();
    at24cxx_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "IIC / AT24C02 TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    printf("24_iic ready\r\n");

    if (at24cxx_check() != 0U)
    {
        lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "24C02 Check Failed!", RED);
        printf("24C02 check failed\r\n");
    }
    else
    {
        lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "24C02 Ready!", BLUE);
        printf("24C02 ready\r\n");
    }

    at24cxx_write(EEPROM_TEST_ADDR, (uint8_t *)g_pattern, EEPROM_BYTE_COUNT);
    at24cxx_write(EEPROM_STR_ADDR, (uint8_t *)g_text, EEPROM_STR_LEN);

    at24cxx_read(EEPROM_TEST_ADDR, g_readback, EEPROM_BYTE_COUNT);
    at24cxx_read(EEPROM_STR_ADDR, (uint8_t *)text_read, EEPROM_STR_LEN);
    text_read[EEPROM_STR_LEN] = '\0';

    for (i = 0U; i < EEPROM_BYTE_COUNT; i++)
    {
        if (g_readback[i] != g_pattern[i])
        {
            byte_ok = false;
        }
    }

    text_ok = (strncmp(text_read, g_text, EEPROM_STR_LEN) == 0);

    sprintf(line, "Bytes: %s", byte_ok ? "OK" : "FAIL");
    lcd_show_string(TEXT_X, 120U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, byte_ok ? BLUE : RED);
    printf("%s\r\n", line);

    sprintf(line, "String: %s", text_read);
    lcd_show_string(TEXT_X, 120U + LINE_HEIGHT, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, text_ok ? BLUE : RED);
    printf("%s (%s)\r\n", line, text_ok ? "OK" : "FAIL");

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
