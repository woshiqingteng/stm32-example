/**
 * @file    main.c
 * @brief   24_i2c_eeprom: AT24C02 EEPROM write/read test over the software IIC bus.
 *          A byte pattern and a string are written, read back, compared and
 *          reported over USART1.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "bsp.h"

#define EEPROM_TEST_ADDR   0U
#define EEPROM_BYTE_COUNT  16U
#define EEPROM_STR_LEN     14U
#define EEPROM_STR_ADDR    (EEPROM_TEST_ADDR + EEPROM_BYTE_COUNT)

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
    eeprom_init();

    printf("24_i2c_eeprom ready\r\n");

    if (eeprom_check() != 0U)
    {
        printf("24C02 check failed\r\n");
    }
    else
    {
        printf("24C02 ready\r\n");
    }

    eeprom_write(EEPROM_TEST_ADDR, (uint8_t *)g_pattern, EEPROM_BYTE_COUNT);
    eeprom_write(EEPROM_STR_ADDR, (uint8_t *)g_text, EEPROM_STR_LEN);

    eeprom_read(EEPROM_TEST_ADDR, g_readback, EEPROM_BYTE_COUNT);
    eeprom_read(EEPROM_STR_ADDR, (uint8_t *)text_read, EEPROM_STR_LEN);
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
    printf("%s\r\n", line);

    sprintf(line, "String: %s", text_read);
    printf("%s (%s)\r\n", line, text_ok ? "OK" : "FAIL");

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
