/**
 * @file    main.c
 * @brief   13_sdram: write/read-back test over the on-board SDRAM.
 */

#include <stdio.h>
#include "bsp.h"
#include "sdram.h"

#define SDRAM_TEST_LEN         512U
#define SDRAM_TEST_OFFSET      0U
#define SDRAM_PATTERN_MUL      7U
#define SDRAM_PATTERN_ADD      3U
#define SDRAM_BLINK_PERIOD_MS  500U

int main(void)
{
    static uint8_t written[SDRAM_TEST_LEN];
    static uint8_t readback[SDRAM_TEST_LEN];
    uint32_t i;
    uint32_t errors = 0U;

    bsp_init();
    sdram_init();

    for (i = 0U; i < SDRAM_TEST_LEN; i++)
    {
        written[i] = (uint8_t)(i * SDRAM_PATTERN_MUL + SDRAM_PATTERN_ADD);
    }

    sdram_write_buffer(written, SDRAM_TEST_OFFSET, SDRAM_TEST_LEN);
    sdram_read_buffer(readback, SDRAM_TEST_OFFSET, SDRAM_TEST_LEN);

    for (i = 0U; i < SDRAM_TEST_LEN; i++)
    {
        if (written[i] != readback[i])
        {
            errors++;
        }
    }

    printf("13_sdram: %u bytes tested, %lu error(s) @0x%08lX\r\n",
           (unsigned)SDRAM_TEST_LEN, (unsigned long)errors,
           (unsigned long)SDRAM_BASE_ADDR);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(SDRAM_BLINK_PERIOD_MS);
    }
}
