/**
 * @file    main.c
 * @brief   37_flash_eeprom: internal flash used as a tiny EEPROM. A string is
 *          written to the last sector, read back and verified.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "bsp.h"

#define FLASH_TEXT        "STM32 FLASH TEST"
#define FLASH_TEXT_SIZE   (sizeof(FLASH_TEXT))
#define FLASH_WORDS       ((FLASH_TEXT_SIZE + 3U) / 4U)

int main(void)
{
    static const char g_text[] = FLASH_TEXT;
    uint32_t          write_buf[FLASH_WORDS];
    uint32_t          read_buf[FLASH_WORDS];
    bool              verified;

    bsp_init();

    memset(write_buf, 0, sizeof(write_buf));
    memcpy(write_buf, g_text, FLASH_TEXT_SIZE);

    printf("37_flash_eeprom ready, addr=0x%08lX, %u words\r\n",
           (unsigned long)STMFLASH_EEPROM_ADDR, (unsigned)FLASH_WORDS);

    stmflash_write(STMFLASH_EEPROM_ADDR, write_buf, FLASH_WORDS);
    stmflash_read(STMFLASH_EEPROM_ADDR, read_buf, FLASH_WORDS);

    verified = (memcmp(read_buf, write_buf, FLASH_TEXT_SIZE) == 0);

    printf("flash verify: %s, read: \"%s\"\r\n",
           verified ? "OK" : "FAIL", (const char *)read_buf);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
