/**
 * @file    main.c
 * @brief   27_spi: W25Qxx SPI NOR flash test. The JEDEC ID is read, sector 0 is
 *          erased, a pattern is written and read back, verified and reported on
 *          the RGB panel and USART1.
 */

#include <stdbool.h>
#include <stdio.h>
#include "bsp.h"

#define NORFLASH_TEST_SECTOR    0U
#define NORFLASH_TEST_ADDR      (NORFLASH_TEST_SECTOR * NORFLASH_SECTOR_SIZE)
#define NORFLASH_TEST_LEN       32U

#define TEXT_X                  30U
#define TEXT_WIDTH              300U

int main(void)
{
    char     line[48];
    uint8_t  pattern[NORFLASH_TEST_LEN];
    uint8_t  readback[NORFLASH_TEST_LEN];
    uint16_t id;
    uint8_t  i;
    bool     ok = true;

    bsp_init();
    sdram_init();
    lcd_init();
    norflash_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "SPI / W25QXX TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    printf("27_spi ready\r\n");

    id = norflash_read_id();
    sprintf(line, "Flash ID: 0x%04X", id);
    lcd_show_string(TEXT_X, 100U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
    printf("%s\r\n", line);

    if ((id == 0U) || (id == 0xFFFFU))
    {
        lcd_show_string(TEXT_X, 120U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "Flash not found!", RED);
        printf("Flash not found!\r\n");
    }
    else
    {
        for (i = 0U; i < NORFLASH_TEST_LEN; i++)
        {
            pattern[i] = (uint8_t)((i * 3U) + 1U);
        }

        norflash_erase_sector(NORFLASH_TEST_SECTOR);
        norflash_write(pattern, NORFLASH_TEST_ADDR, NORFLASH_TEST_LEN);
        norflash_read(readback, NORFLASH_TEST_ADDR, NORFLASH_TEST_LEN);

        for (i = 0U; i < NORFLASH_TEST_LEN; i++)
        {
            if (readback[i] != pattern[i])
            {
                ok = false;
            }
        }

        sprintf(line, "Write/Read: %s", ok ? "OK" : "FAIL");
        lcd_show_string(TEXT_X, 120U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, ok ? BLUE : RED);
        printf("%s\r\n", line);

        for (i = 0U; i < 8U; i++)
        {
            sprintf(line + (i * 3U), "%02X ", readback[i]);
        }
        line[24] = '\0';
        lcd_show_string(TEXT_X, 140U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
        printf("Data: %s\r\n", line);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
