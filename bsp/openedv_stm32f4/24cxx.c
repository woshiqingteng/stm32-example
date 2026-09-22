/**
 * @file    24cxx.c
 * @brief   AT24Cxx series I2C EEPROM driver (single byte and block access).
 *
 * Address encoding follows the vendor driver: devices larger than 24C16 send a
 * 16-bit address as two bytes, smaller ones fold the upper address bits into
 * the slave address word.
 */

#include "stm32f4xx_hal.h"
#include "iic.h"
#include "24cxx.h"
#include "delay.h"

#define AT24CXX_WRITE_DELAY_MS 10U
#define AT24CXX_CHECK_VALUE    0x55U

void at24cxx_init(void)
{
    iic_init();
}

uint8_t at24cxx_read_one_byte(uint16_t addr)
{
    uint8_t temp = 0;

    iic_start();

    if (EE_TYPE > AT24C16)
    {
        iic_send_byte(0xA0U);
        iic_wait_ack();
        iic_send_byte((uint8_t)(addr >> 8));
    }
    else
    {
        iic_send_byte((uint8_t)(0xA0U + ((addr >> 8) << 1)));
    }

    iic_wait_ack();
    iic_send_byte((uint8_t)(addr % 256U));
    iic_wait_ack();

    iic_start();
    iic_send_byte(0xA1U);
    iic_wait_ack();
    temp = iic_read_byte(0);
    iic_stop();

    return temp;
}

void at24cxx_write_one_byte(uint16_t addr, uint8_t data)
{
    iic_start();

    if (EE_TYPE > AT24C16)
    {
        iic_send_byte(0xA0U);
        iic_wait_ack();
        iic_send_byte((uint8_t)(addr >> 8));
    }
    else
    {
        iic_send_byte((uint8_t)(0xA0U + ((addr >> 8) << 1)));
    }

    iic_wait_ack();
    iic_send_byte((uint8_t)(addr % 256U));
    iic_wait_ack();

    iic_send_byte(data);
    iic_wait_ack();
    iic_stop();

    delay_ms(AT24CXX_WRITE_DELAY_MS);
}

uint8_t at24cxx_check(void)
{
    uint8_t temp;
    uint16_t addr = EE_TYPE;

    temp = at24cxx_read_one_byte(addr);

    if (temp == AT24CXX_CHECK_VALUE)
    {
        return 0;
    }

    at24cxx_write_one_byte(addr, AT24CXX_CHECK_VALUE);
    temp = at24cxx_read_one_byte(addr);

    return (temp == AT24CXX_CHECK_VALUE) ? 0U : 1U;
}

void at24cxx_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    while (datalen-- != 0U)
    {
        *pbuf++ = at24cxx_read_one_byte(addr++);
    }
}

void at24cxx_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    while (datalen-- != 0U)
    {
        at24cxx_write_one_byte(addr, *pbuf);
        addr++;
        pbuf++;
    }
}
