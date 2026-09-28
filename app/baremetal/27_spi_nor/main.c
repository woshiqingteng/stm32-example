/**
 * @file    main.c
 * @brief   27_spi_nor: W25Qxx SPI NOR flash test. The JEDEC ID is read, sector 0 is
 *          erased, a pattern is written and read back, verified and reported
 *          over USART1.
 */

#include <stdbool.h>
#include <stdio.h>

#include "bsp.h"
#include "nor.h"

#define NOR_TEST_SECTOR    0U
#define NOR_TEST_ADDR      (NOR_TEST_SECTOR * NOR_SECTOR_SIZE_BYTE)
#define NOR_TEST_LEN_BYTE       32U

int main(void)
{
    char     line[48];
    uint8_t  pattern[NOR_TEST_LEN_BYTE];
    uint8_t  readback[NOR_TEST_LEN_BYTE];
    uint16_t id;
    uint8_t  i;
    bool     ok = true;

    bsp_init();
    nor_init();

    printf(APP_BANNER "\r\n");

    id = nor_read_id();
    sprintf(line, "Flash ID: 0x%04X", id);
    printf("%s\r\n", line);

    if ((id == 0U) || (id == 0xFFFFU))
    {
        printf("Flash not found!\r\n");
    }
    else
    {
        for (i = 0U; i < NOR_TEST_LEN_BYTE; i++)
        {
            pattern[i] = (uint8_t)((i * 3U) + 1U);
        }

        nor_erase_sector(NOR_TEST_SECTOR);
        nor_write(pattern, NOR_TEST_ADDR, NOR_TEST_LEN_BYTE);
        nor_read(readback, NOR_TEST_ADDR, NOR_TEST_LEN_BYTE);

        for (i = 0U; i < NOR_TEST_LEN_BYTE; i++)
        {
            if (readback[i] != pattern[i])
            {
                ok = false;
            }
        }

        sprintf(line, "Write/Read: %s", ok ? "OK" : "FAIL");
        printf("%s\r\n", line);

        for (i = 0U; i < 8U; i++)
        {
            sprintf(line + (i * 3U), "%02X ", readback[i]);
        }
        line[24] = '\0';
        printf("Data: %s\r\n", line);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
