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

#define FLASH_TEXT_X      30U
#define FLASH_TEXT_WIDTH  280U

int main(void)
{
    static const char g_text[] = FLASH_TEXT;
    uint32_t          write_buf[FLASH_WORDS];
    uint32_t          read_buf[FLASH_WORDS];
    bool              verified;

    bsp_init();
    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(FLASH_TEXT_X, 50U, FLASH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(FLASH_TEXT_X, 70U, FLASH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "FLASH EEPROM TEST", RED);
    lcd_show_string(FLASH_TEXT_X, 90U, FLASH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    memset(write_buf, 0, sizeof(write_buf));
    memcpy(write_buf, g_text, FLASH_TEXT_SIZE);

    printf("37_flash_eeprom ready, addr=0x%08lX, %u words\r\n",
           (unsigned long)STMFLASH_EEPROM_ADDR, (unsigned)FLASH_WORDS);

    stmflash_write(STMFLASH_EEPROM_ADDR, write_buf, FLASH_WORDS);
    stmflash_read(STMFLASH_EEPROM_ADDR, read_buf, FLASH_WORDS);

    verified = (memcmp(read_buf, write_buf, FLASH_TEXT_SIZE) == 0);

    printf("flash verify: %s, read: \"%s\"\r\n",
           verified ? "OK" : "FAIL", (const char *)read_buf);

    lcd_show_string(FLASH_TEXT_X, 130U, FLASH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Read back:", BLACK);
    lcd_show_string(FLASH_TEXT_X, 150U, FLASH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, (const char *)read_buf, BLUE);
    lcd_show_string(FLASH_TEXT_X, 180U, FLASH_TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                    verified ? "VERIFY OK" : "VERIFY FAIL", verified ? GREEN : RED);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
