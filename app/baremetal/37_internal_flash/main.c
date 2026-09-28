/**
 * @file    main.c
 * @brief   37_internal_flash: internal flash used as a tiny EEPROM. A string is
 *          written to the last sector, read back and verified.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bsp.h"

#define FLASH_TEXT        "STM32 FLASH TEST"
#define FLASH_TEXT_SIZE_BYTE   (sizeof(FLASH_TEXT))
#define FLASH_WORD_COUNT       ((FLASH_TEXT_SIZE_BYTE + 3U) / 4U)

int main(void)
{
    static const char g_text[] = FLASH_TEXT;
    uint32_t          write_buf[FLASH_WORD_COUNT];
    uint32_t          read_buf[FLASH_WORD_COUNT];
    bool              verified;

    bsp_init();
    printf(APP_BANNER "\r\n");

    memset(write_buf, 0, sizeof(write_buf));
    memcpy(write_buf, g_text, FLASH_TEXT_SIZE_BYTE);

    printf("37_internal_flash ready, addr=0x%08lX, %u words\r\n",
           (unsigned long)INTERNAL_FLASH_EEPROM_ADDR, (unsigned)FLASH_WORD_COUNT);

    internal_flash_write(INTERNAL_FLASH_EEPROM_ADDR, write_buf, FLASH_WORD_COUNT);
    internal_flash_read(INTERNAL_FLASH_EEPROM_ADDR, read_buf, FLASH_WORD_COUNT);

    verified = (memcmp(read_buf, write_buf, FLASH_TEXT_SIZE_BYTE) == 0);

    printf("flash verify: %s, read: \"%s\"\r\n",
           verified ? "OK" : "FAIL", (const char *)read_buf);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
