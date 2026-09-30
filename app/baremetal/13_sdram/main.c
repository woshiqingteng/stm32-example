/**
 * @file    main.c
 * @brief   13_sdram: SDRAM capacity test. KEY0 walks every 16 KB block to
 *          measure the capacity across the whole part; KEY1 dumps a preloaded
 *          pattern. Results are reported over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "sdram.h"

#define SDRAM_SIZE_BYTE    (32U * 1024U * 1024U)
#define SDRAM_BLOCK_STEP_BYTE    (16U * 1024U)
#define SDRAM_DATA_WORD_COUNT    4096U
#define SDRAM_LOOP_MS       10U
#define SDRAM_LED_TICK_COUNT     20U
#define SDRAM_DUMP_PER_LINE   8U
#define SDRAM_DUMP_PER_PAGE   128U

/* The pattern region must be a whole number of pages and lines. */
_Static_assert((SDRAM_DATA_WORD_COUNT % SDRAM_DUMP_PER_PAGE) == 0U, "SDRAM dump: partial page");
_Static_assert((SDRAM_DUMP_PER_PAGE % SDRAM_DUMP_PER_LINE) == 0U, "SDRAM dump: partial line");

static volatile uint16_t *const g_sdram = (volatile uint16_t *)SDRAM_BASE_ADDR;
static uint32_t g_dump_pos = 0U;   /* next value index to dump */

static void sdram_prefill(void)
{
    uint32_t t;

    for (t = 0U; t < SDRAM_DATA_WORD_COUNT; t++)
    {
        g_sdram[t] = (uint16_t)t;
    }
}

static void sdram_capacity_test(void)
{
    volatile uint32_t *base = (volatile uint32_t *)SDRAM_BASE_ADDR;
    uint32_t           i;
    uint32_t           temp = 0U;
    uint32_t           sval = 0U;
    uint32_t           block = 0U;
    uint32_t           cap_kb;

    /* Write an incrementing counter to every 16 KB block. */
    for (i = 0U; i < SDRAM_SIZE_BYTE; i += SDRAM_BLOCK_STEP_BYTE)
    {
        base[i / 4U] = temp;
        temp++;
    }

    /* Count the blocks reading back as strictly increasing values. */
    for (i = 0U; i < SDRAM_SIZE_BYTE; i += SDRAM_BLOCK_STEP_BYTE)
    {
        temp = base[i / 4U];

        if (i == 0U)
        {
            sval  = temp;
            block = 1U;
        }
        else if (temp <= sval)      /* address mirroring: counter wrapped */
        {
            break;
        }
        else
        {
            block++;
        }
    }

    cap_kb = block * (SDRAM_BLOCK_STEP_BYTE / 1024U);
    printf("SDRAM Capacity:%luKB\r\n", (unsigned long)cap_kb);
}

static void sdram_data_dump(void)
{
    uint32_t end = g_dump_pos + SDRAM_DUMP_PER_PAGE;
    uint32_t i;
    uint32_t j;

    printf("dump [%lu..%lu) @0x%08lX\r\n",
           (unsigned long)g_dump_pos, (unsigned long)end,
           (unsigned long)(SDRAM_BASE_ADDR + (g_dump_pos * 2U)));

    for (i = g_dump_pos; i < end; i += SDRAM_DUMP_PER_LINE)
    {
        printf("%08lX:", (unsigned long)i);

        for (j = 0U; j < SDRAM_DUMP_PER_LINE; j++)
        {
            printf(" %04X", (unsigned)g_sdram[i + j]);
        }

        printf("\r\n");
    }

    g_dump_pos = (end >= SDRAM_DATA_WORD_COUNT) ? 0U : end;

    if (g_dump_pos == 0U)
    {
        printf("dump complete\r\n");
    }
}

int main(void)
{
    uint8_t  key;
    uint8_t  blink = 0U;

    bsp_init();
    sdram_init();

    sdram_prefill();

    printf(APP_BANNER "\r\n");
    printf("KEY0: capacity test  KEY1: data dump\r\n");

    for (;;)
    {
        key = (uint8_t)key_scan(false);

        if (key == KEY0)
        {
            printf("capacity test running...\r\n");
            sdram_capacity_test();
            sdram_prefill();          /* restore the prefill pattern for the dump */
        }
        else if (key == KEY1)
        {
            sdram_data_dump();
        }
        else
        {
            delay_ms(SDRAM_LOOP_MS);
        }

        blink++;
        if (blink >= SDRAM_LED_TICK_COUNT)
        {
            blink = 0U;
            led_toggle(LED0);
        }
    }
}
