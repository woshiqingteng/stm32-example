/**
 * @file    eeprom_at24c02.c
 * @brief   AT24C02 I2C EEPROM (chip driver).
 *
 * Address encoding follows the vendor driver: devices larger than 24C16 send a
 * 16-bit address as two bytes, smaller ones fold the upper address bits into
 * the slave address word.
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "eeprom_at24c02.h"
#include "delay.h"

#define AT24C02_WRITE_DELAY_MS 10U
#define AT24C02_CHECK_VALUE    0x55U

void eeprom_at24c02_init(void)
{
    i2c_init();
}

uint8_t eeprom_at24c02_read_one_byte(uint16_t addr)
{
    uint8_t temp = 0;

    i2c_start();

    if (EE_TYPE > AT24C16)
    {
        i2c_send_byte(0xA0U);
        i2c_wait_ack();
        i2c_send_byte((uint8_t)(addr >> 8));
    }
    else
    {
        i2c_send_byte((uint8_t)(0xA0U + ((addr >> 8) << 1)));
    }

    i2c_wait_ack();
    i2c_send_byte((uint8_t)(addr % 256U));
    i2c_wait_ack();

    i2c_start();
    i2c_send_byte(0xA1U);
    i2c_wait_ack();
    temp = i2c_read_byte(0);
    i2c_stop();

    return temp;
}

void eeprom_at24c02_write_one_byte(uint16_t addr, uint8_t data)
{
    i2c_start();

    if (EE_TYPE > AT24C16)
    {
        i2c_send_byte(0xA0U);
        i2c_wait_ack();
        i2c_send_byte((uint8_t)(addr >> 8));
    }
    else
    {
        i2c_send_byte((uint8_t)(0xA0U + ((addr >> 8) << 1)));
    }

    i2c_wait_ack();
    i2c_send_byte((uint8_t)(addr % 256U));
    i2c_wait_ack();

    i2c_send_byte(data);
    i2c_wait_ack();
    i2c_stop();

    delay_ms(AT24C02_WRITE_DELAY_MS);
}

uint8_t eeprom_at24c02_check(void)
{
    uint8_t temp;
    uint16_t addr = EE_TYPE;

    temp = eeprom_at24c02_read_one_byte(addr);

    if (temp == AT24C02_CHECK_VALUE)
    {
        return 0;
    }

    eeprom_at24c02_write_one_byte(addr, AT24C02_CHECK_VALUE);
    temp = eeprom_at24c02_read_one_byte(addr);

    return (temp == AT24C02_CHECK_VALUE) ? 0U : 1U;
}
