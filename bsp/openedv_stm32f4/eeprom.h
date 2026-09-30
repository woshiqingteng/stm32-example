/**
 * @file    eeprom.h
 * @brief   I2C EEPROM storage: byte and block access.
 */

#ifndef BSP_EEPROM_H
#define BSP_EEPROM_H

#include <stdint.h>

/** @brief  Initialise the EEPROM (and its shared software IIC bus). */
void eeprom_init(void);

/** @brief  Probe the device. @return 0 on success. */
uint8_t eeprom_check(void);

uint8_t eeprom_read_one_byte(uint16_t addr);
void    eeprom_write_one_byte(uint16_t addr, uint8_t data);

void eeprom_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen);
void eeprom_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen);

#endif /* BSP_EEPROM_H */
