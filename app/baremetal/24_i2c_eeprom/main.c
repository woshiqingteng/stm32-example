/**
 * @file    main.c
 * @brief   24_i2c_eeprom: AT24C02 EEPROM over the software IIC bus. KEY1 writes
 *          a string, KEY0 reads it back, verifies and reports over USART1;
 *          WK_UP clears the stored string.
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

static const char    g_text[] = "STM32 IIC TEST";
static const uint8_t g_zero[sizeof(g_text)] = { 0U };

/* Write the test string. */
static void eeprom_write_test(void)
{
    eeprom_write(EEPROM_TEST_ADDR, (const uint8_t *)g_text, (uint16_t)sizeof(g_text));
    printf("24C02 write done\r\n");
}

/* Read the string back, verify it and report. */
static void eeprom_read_verify(void)
{
    char text_read[sizeof(g_text) + 1U];

    eeprom_read(EEPROM_TEST_ADDR, (uint8_t *)text_read, (uint16_t)sizeof(g_text));
    text_read[sizeof(g_text)] = '\0';

    printf("String: %s (%s)\r\n", text_read,
           (memcmp(text_read, g_text, sizeof(g_text)) == 0) ? "OK" : "FAIL");
}

/* Erase the stored string (write zeros). */
static void eeprom_clear_test(void)
{
    eeprom_write(EEPROM_TEST_ADDR, g_zero, (uint16_t)sizeof(g_zero));
    printf("24C02 cleared\r\n");
}

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    eeprom_init();
    printf(APP_BANNER "\r\n");

    /* Block until the EEPROM answers. */
    while (eeprom_check() != 0U)
    {
        printf("24C02 check failed\r\n");
    }
    printf("KEY1: write  KEY0: read  WKUP: clear\r\n");

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY1)
        {
            eeprom_write_test();
        }
        else if (key == KEY0)
        {
            eeprom_read_verify();
        }
        else if (key == KEY_WKUP)
        {
            eeprom_clear_test();
        }

        if ((++blink % LED_BLINK_TICKS) == 0U)
        {
            led_toggle(LED0);          /* run indicator, ~500 ms */
        }
        delay_ms(APP_LOOP_MS);
    }
}
