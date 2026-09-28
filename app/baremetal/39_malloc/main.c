/**
 * @file    main.c
 * @brief   39_malloc: ALIENTEK memory manager test. A pattern is allocated,
 *          written, verified and freed in each of the three banks (internal
 *          SRAM, CCM and the external SDRAM) and the usage is reported on
 *          USART1.
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "malloc.h"
#define TEST_SIZE_BYTE       2048U
#define BLINK_PERIOD_MS 500U

static const char *const g_bank_name[SRAMBANK] = {"SRAMIN", "SRAMCCM", "SRAMEX"};

static uint8_t g_pattern[TEST_SIZE_BYTE];
static uint8_t g_readback[TEST_SIZE_BYTE];

static void pattern_fill(uint8_t *buf, uint32_t len, uint8_t seed)
{
    uint32_t i;

    for (i = 0U; i < len; i++)
    {
        buf[i] = (uint8_t)(i ^ seed);
    }
}

static uint32_t pattern_errors(const uint8_t *buf, uint32_t len, uint8_t seed)
{
    uint32_t i;
    uint32_t errors = 0U;

    for (i = 0U; i < len; i++)
    {
        if (buf[i] != (uint8_t)(i ^ seed))
        {
            errors++;
        }
    }

    return errors;
}

int main(void)
{
    uint8_t  bank;
    uint8_t *p;
    uint16_t used;
    uint32_t errors;

    bsp_init();
    sdram_init();

    my_mem_init(SRAMIN);
    my_mem_init(SRAMCCM);
    my_mem_init(SRAMEX);


    printf(APP_BANNER "\r\n");

    for (bank = 0U; bank < SRAMBANK; bank++)
    {
        pattern_fill(g_pattern, TEST_SIZE_BYTE, (uint8_t)(bank * 3U + 1U));

        p = mymalloc(bank, TEST_SIZE_BYTE);

        if (p == NULL)
        {
            printf("%s malloc failed\r\n", g_bank_name[bank]);
            continue;
        }

        memcpy(p, g_pattern, TEST_SIZE_BYTE);
        memcpy(g_readback, p, TEST_SIZE_BYTE);

        errors = pattern_errors(g_readback, TEST_SIZE_BYTE, (uint8_t)(bank * 3U + 1U));
        myfree(bank, p);

        used = my_mem_perused(bank);
        printf("%s: %s  used %u.%u%%\r\n", g_bank_name[bank],
               (errors == 0U) ? "OK  " : "FAIL",
               (unsigned)(used / 10U), (unsigned)(used % 10U));
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
