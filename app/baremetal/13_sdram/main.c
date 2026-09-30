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
#define SDRAM_DATA_WORD_COUNT    250000U
#define SDRAM_LOOP_MS       10U
#define SDRAM_LED_TICK_COUNT     20U

static uint16_t *const g_sdram = (uint16_t *)SDRAM_BASE_ADDR;

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
    uint32_t           cap_kb = 0U;

    for (i = 0U; i < SDRAM_SIZE_BYTE; i += SDRAM_BLOCK_STEP_BYTE)
    {
        base[i / 4U] = temp;
        temp++;
    }

    for (i = 0U; i < SDRAM_SIZE_BYTE; i += SDRAM_BLOCK_STEP_BYTE)
    {
        temp = base[i / 4U];

        if (i == 0U)
        {
            sval = temp;
        }
        else if (temp <= sval)
        {
            break;
        }

        cap_kb = (temp - sval + 1U) * (SDRAM_BLOCK_STEP_BYTE / 1024U);
    }

    printf("SDRAM Capacity:%luKB\r\n", (unsigned long)cap_kb);
}

static void sdram_data_dump(void)
{
    uint32_t t;

    for (t = 0U; t < SDRAM_DATA_WORD_COUNT; t++)
    {
        printf("testsdram[%lu]:%u\r\n", (unsigned long)t, (unsigned)g_sdram[t]);
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
            sdram_capacity_test();
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
