/**
 * @file    main.c
 * @brief   13_sdram: full-capacity SDRAM test. KEY0 walks every 16 KB block to
 *          exercise the address lines across the whole 32 MB part; KEY1 checks a
 *          512 KB data pattern. Results are reported on USART1.
 */

#include <stdio.h>
#include "bsp.h"
#include "sdram.h"

#define SDRAM_TEST_BLOCKS   2048U                 /* 2048 * 16 KB = 32 MB */
#define SDRAM_BLOCK_STEP    (16U * 1024U)
#define SDRAM_PATTERN_WORDS (512U * 1024U / 4U)   /* 512 KB of words */
#define SDRAM_PATTERN_MUL   7U
#define SDRAM_PATTERN_ADD   3U
#define SDRAM_BLINK_MS      500U

static uint32_t sdram_capacity_test(void)
{
    volatile uint32_t *base = (volatile uint32_t *)SDRAM_BASE_ADDR;
    uint32_t           i;
    uint32_t           errors = 0U;

    for (i = 0U; i < SDRAM_TEST_BLOCKS; i++)
    {
        base[i * (SDRAM_BLOCK_STEP / 4U)] = i;
    }

    for (i = 0U; i < SDRAM_TEST_BLOCKS; i++)
    {
        if (base[i * (SDRAM_BLOCK_STEP / 4U)] != i)
        {
            errors++;
        }
    }

    return errors;
}

static uint32_t sdram_pattern_test(void)
{
    volatile uint32_t *base = (volatile uint32_t *)SDRAM_BASE_ADDR;
    uint32_t           i;
    uint32_t           errors = 0U;

    for (i = 0U; i < SDRAM_PATTERN_WORDS; i++)
    {
        base[i] = (i * SDRAM_PATTERN_MUL) + SDRAM_PATTERN_ADD;
    }

    for (i = 0U; i < SDRAM_PATTERN_WORDS; i++)
    {
        if (base[i] != ((i * SDRAM_PATTERN_MUL) + SDRAM_PATTERN_ADD))
        {
            errors++;
        }
    }

    return errors;
}

int main(void)
{
    bsp_init();
    sdram_init();

    printf("13_sdram ready: KEY0=capacity 32MB, KEY1=pattern 512KB\r\n");

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY0)
        {
            uint32_t errors = sdram_capacity_test();

            printf("capacity test: %u blocks, %lu error(s)\r\n",
                   (unsigned)SDRAM_TEST_BLOCKS, (unsigned long)errors);
        }
        else if (key == KEY1)
        {
            uint32_t errors = sdram_pattern_test();

            printf("pattern test: %u bytes, %lu error(s)\r\n",
                   (unsigned)(SDRAM_PATTERN_WORDS * 4U), (unsigned long)errors);
        }
        else
        {
            /* no key */
        }

        led_toggle(LED0);
        delay_ms(SDRAM_BLINK_MS);
    }
}
