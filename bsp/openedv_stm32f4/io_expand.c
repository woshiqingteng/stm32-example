/**
 * @file    io_expand.c
 * @brief   8-bit I2C IO expander (delegates to the chip driver).
 */

#include "io_expand.h"
#include "io_expand_pcf8574t.h"

uint8_t io_expand_init(void)
{
    return io_expand_pcf8574t_init();
}

uint8_t io_expand_read(void)
{
    return io_expand_pcf8574t_read();
}

void io_expand_write(uint8_t data)
{
    io_expand_pcf8574t_write(data);
}

uint8_t io_expand_read_bit(uint8_t bit)
{
    return (uint8_t)((io_expand_read() >> bit) & 0x01U);
}

void io_expand_write_bit(uint8_t bit, uint8_t sta)
{
    uint8_t data = io_expand_read();

    if (sta == 0U)
    {
        data &= (uint8_t)~(1U << bit);
    }
    else
    {
        data |= (uint8_t)(1U << bit);
    }

    io_expand_write(data);
}

bool io_expand_int_asserted(void)
{
    return io_expand_pcf8574t_int_asserted();
}
