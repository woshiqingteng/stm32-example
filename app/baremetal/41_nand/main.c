/**
 * @file    main.c
 * @brief   41_nand: NAND FLASH test over the vendor FTL (vendor experiment 41).
 *
 * The FTL (ftl.c) turns the raw NAND into logical sectors with bad-block
 * handling; this app exercises it from the serial console:
 *   KEY0 -> read sector 2 and dump it in hex.
 *   KEY1 -> write a pattern to sector 2.
 *   KEY2 -> restore sector 2 from a backup taken at boot.
 * The LED blinks as a heartbeat and NAND init failures are reported on USART1.
 */

#include <stdint.h>
#include <stdio.h>
#include "bsp.h"
#include "malloc.h"
#include "nand.h"
#include "ftl.h"

#define TEST_SECTOR   2U

int main(void)
{
    key_id_t key;
    uint16_t t = 0U;
    uint16_t i;
    uint8_t *buf;
    uint8_t *backbuf;
    uint8_t  ok;

    bsp_init();

    my_mem_init(SRAMIN);

    printf("41_nand ready\r\n");

    while (ftl_init() != 0U)
    {
        printf("NAND Error!\r\nPlease Check!\r\n");
        led_toggle(LED0);
        delay_ms(500);
    }

    backbuf = (uint8_t *)mymalloc(SRAMIN, NAND_ECC_SECTOR_SIZE);
    buf     = (uint8_t *)mymalloc(SRAMIN, NAND_ECC_SECTOR_SIZE);

    if ((backbuf == 0) || (buf == 0))
    {
        printf("malloc failed\r\n");

        for (;;)
        {
            led_toggle(LED0);
            delay_ms(200);
        }
    }

    printf("NAND Size:%dMB\r\n",
           (int)(((uint32_t)nand_dev.block_totalnum / 1024U) *
                 ((uint32_t)nand_dev.page_mainsize / 1024U) *
                 (uint32_t)nand_dev.block_pagenum));

    /* Keep a pristine copy of sector 2 so KEY2 can restore it. */
    (void)ftl_read_sectors(backbuf, TEST_SECTOR, NAND_ECC_SECTOR_SIZE, 1U);

    for (;;)
    {
        key = key_scan(false);

        switch (key)
        {
            case KEY0:                                          /* read sector */
                ok = ftl_read_sectors(buf, TEST_SECTOR, NAND_ECC_SECTOR_SIZE, 1U);

                if (ok == 0U)
                {
                    printf("Sector %u data is:\r\n", (unsigned int)TEST_SECTOR);

                    for (i = 0U; i < NAND_ECC_SECTOR_SIZE; i++)
                    {
                        printf("%x ", buf[i]);
                    }

                    printf("\r\ndata end.\r\n");
                }
                break;

            case KEY1:                                          /* write sector */
                for (i = 0U; i < NAND_ECC_SECTOR_SIZE; i++)
                {
                    buf[i] = (uint8_t)(i + t);
                }

                ok = ftl_write_sectors(buf, TEST_SECTOR, NAND_ECC_SECTOR_SIZE, 1U);
                printf(ok == 0U ? "Write data successed\r\n" : "Write data failed\r\n");
                break;

            case KEY2:                                          /* restore sector */
                ok = ftl_write_sectors(backbuf, TEST_SECTOR, NAND_ECC_SECTOR_SIZE, 1U);
                printf(ok == 0U ? "Recovering data OK\r\n" : "Recovering data failed\r\n");
                break;

            default:
                break;
        }

        t++;

        if (t == 20U)
        {
            t = 0U;
            led_toggle(LED0);
        }

        delay_ms(10);
    }
}
