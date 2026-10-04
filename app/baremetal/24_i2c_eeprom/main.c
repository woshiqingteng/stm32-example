/**
 * @file    main.c
 * @brief   24_i2c_eeprom: AT24C02 EEPROM over the software IIC bus. KEY1 writes
 *          a byte pattern and a string, KEY0 reads them back, verifies and
 *          reports over USART1.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "eeprom.h"

#define EEPROM_TEST_ADDR   0U

#define APP_LOOP_MS        10U
#define LED_BLINK_MS       500U
#define LED_BLINK_TICKS    (LED_BLINK_MS / APP_LOOP_MS)

static const uint8_t g_pattern[] =
{
    0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U,
    0x88U, 0x99U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU
};

static const char g_text[] = "STM32 IIC TEST";

/* Write the test data: the byte pattern followed by the string. */
static void eeprom_write_test(void)
{
    eeprom_write(EEPROM_TEST_ADDR, g_pattern, (uint16_t)sizeof(g_pattern));
    eeprom_write((uint16_t)(EEPROM_TEST_ADDR + sizeof(g_pattern)),
                 (const uint8_t *)g_text, (uint16_t)sizeof(g_text));
    printf("24C02 write done\r\n");
}

/* Read the test data back, verify it and report the result. */
static void eeprom_read_verify(void)
{
    uint8_t  readback[sizeof(g_pattern)];
    char     text_read[sizeof(g_text) + 1U];
    uint16_t i;
    bool     byte_ok = true;
    bool     text_ok;

    eeprom_read(EEPROM_TEST_ADDR, readback, (uint16_t)sizeof(g_pattern));
    eeprom_read((uint16_t)(EEPROM_TEST_ADDR + sizeof(g_pattern)),
                (uint8_t *)text_read, (uint16_t)sizeof(g_text));
    text_read[sizeof(g_text)] = '\0';

    for (i = 0U; i < (uint16_t)sizeof(g_pattern); i++)
    {
        if (readback[i] != g_pattern[i])
        {
            byte_ok = false;
        }
    }
    text_ok = (memcmp(text_read, g_text, sizeof(g_text)) == 0);

    printf("Bytes: %s\r\n", byte_ok ? "OK" : "FAIL");
    printf("String: %s (%s)\r\n", text_read, text_ok ? "OK" : "FAIL");
}

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    eeprom_init();
    printf(APP_BANNER "\r\n");

    /* Block until the EEPROM answers (KEY1 check). */
    while (eeprom_check() != 0U)
    {
        printf("24C02 check failed\r\n");
        led_toggle(LED0);
        delay_ms(500U);
    }
    printf("24C02 ready\r\nKEY1: write  KEY0: read\r\n");

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY1)
        {
            eeprom_write_test();
            led_toggle(LED1);          /* action indicator */
        }
        else if (key == KEY0)
        {
            eeprom_read_verify();
            led_toggle(LED1);          /* action indicator */
        }

        if ((++blink % LED_BLINK_TICKS) == 0U)
        {
            led_toggle(LED0);          /* run indicator, ~500 ms */
        }
        delay_ms(APP_LOOP_MS);
    }
}
