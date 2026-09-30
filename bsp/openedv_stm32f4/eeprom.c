/**
 * @file    eeprom.c
 * @brief   I2C EEPROM storage: device-independent byte and block access.
 */

#include "eeprom.h"
#include "eeprom_at24cxx.h"

void eeprom_init(void)
{
    eeprom_at24cxx_init();
}

uint8_t eeprom_check(void)
{
    return eeprom_at24cxx_check();
}

uint8_t eeprom_read_one_byte(uint16_t addr)
{
    return eeprom_at24cxx_read_one_byte(addr);
}

void eeprom_write_one_byte(uint16_t addr, uint8_t data)
{
    eeprom_at24cxx_write_one_byte(addr, data);
}

void eeprom_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    while (datalen-- != 0U)
    {
        *pbuf++ = eeprom_read_one_byte(addr++);
    }
}

void eeprom_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    while (datalen-- != 0U)
    {
        eeprom_write_one_byte(addr, *pbuf);
        addr++;
        pbuf++;
    }
}
