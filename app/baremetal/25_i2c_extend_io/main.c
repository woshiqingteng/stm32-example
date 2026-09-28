/**
 * @file    main.c
 * @brief   25_i2c_extend_io: PCF8574 8-bit IO expander test. Alternating patterns
 *          are written and read back; the expander INT line and the EX_IO input
 *          are also polled. Results are reported over USART1.
 */

#include <stdio.h>

#include "bsp.h"

#define PATTERN_PERIOD_MS  500U

static const uint8_t g_patterns[] = { 0xAAU, 0x55U };

int main(void)
{
    uint8_t idx = 0U;
    uint8_t status;

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (io_expand_init() != 0U)
    {
        printf("PCF8574 check failed\r\n");
    }
    else
    {
        printf("PCF8574 ready\r\n");
    }

    printf("25_i2c_extend_io ready\r\n");

    for (;;)
    {
        uint8_t write_val = g_patterns[idx];
        uint8_t read_val;

        io_expand_write_byte(write_val);
        read_val = io_expand_read_byte();

        printf("Write:0x%02X Read:0x%02X\r\n", write_val, read_val);

        status = (io_expand_int_asserted()) ? 1U : 0U;
        printf("INT:%u EX_IO:%u\r\n", status, (unsigned)io_expand_read_bit(PCF8574_EX_IO));

        /* Buzzer demo: pulse BEEP via the expander bit API. */
        io_expand_write_bit(PCF8574_BEEP_IO, 1U);
        delay_ms(200U);
        io_expand_write_bit(PCF8574_BEEP_IO, 0U);
        printf("BEEP pulse\r\n");

        idx ^= 1U;
        led_toggle(LED0);
        delay_ms(PATTERN_PERIOD_MS);
    }
}
