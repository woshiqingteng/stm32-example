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

#define TEST_SIZE       2048U
#define BLINK_PERIOD_MS 500U

static const char *const g_bank_name[SRAMBANK] = {"SRAMIN", "SRAMCCM", "SRAMEX"};

static uint8_t g_pattern[TEST_SIZE];
static uint8_t g_readback[TEST_SIZE];

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
    char     line[64];

    bsp_init();
    sdram_init();

    my_mem_init(SRAMIN);
    my_mem_init(SRAMCCM);
    my_mem_init(SRAMEX);


    printf("39_malloc ready\r\n");

    for (bank = 0U; bank < SRAMBANK; bank++)
    {
        pattern_fill(g_pattern, TEST_SIZE, (uint8_t)(bank * 3U + 1U));

        p = mymalloc(bank, TEST_SIZE);

        if (p == NULL)
        {
            sprintf(line, "%s malloc failed", g_bank_name[bank]);
            printf("%s\r\n", line);
            continue;
        }

        memcpy(p, g_pattern, TEST_SIZE);
        memcpy(g_readback, p, TEST_SIZE);

        errors = pattern_errors(g_readback, TEST_SIZE, (uint8_t)(bank * 3U + 1U));
        myfree(bank, p);

        used = my_mem_perused(bank);
        sprintf(line, "%s: %s  used %u.%u%%", g_bank_name[bank],
                (errors == 0U) ? "OK  " : "FAIL",
                (unsigned)(used / 10U), (unsigned)(used % 10U));
        printf("%s\r\n", line);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
