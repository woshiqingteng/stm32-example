/**
 * @file    eeprom.c
 * @brief   I2C EEPROM storage: device-independent block access.
 */

#include "eeprom.h"
#include "eeprom_at24c02.h"

void eeprom_init(void)
{
    eeprom_at24c02_init();
}

uint8_t eeprom_check(void)
{
    return eeprom_at24c02_check();
}

void eeprom_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    eeprom_at24c02_read(addr, pbuf, datalen);
}

void eeprom_write(uint16_t addr, const uint8_t *pbuf, uint16_t datalen)
{
    eeprom_at24c02_write(addr, pbuf, datalen);
}
