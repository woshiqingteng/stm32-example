/**
 * @file    eeprom.h
 * @brief   AT24Cxx series I2C EEPROM driver.
 */

#ifndef BSP_EEPROM_H
#define BSP_EEPROM_H

#include <stdint.h>

#define AT24C01     127U
#define AT24C02     255U
#define AT24C04     511U
#define AT24C08     1023U
#define AT24C16     2047U
#define AT24C32     4095U
#define AT24C64     8191U
#define AT24C128    16383U
#define AT24C256    32767U

/** @brief  Device fitted on the ALIENTEK F429 board. */
#define EE_TYPE     AT24C02

/** @brief  Initialise the shared software IIC bus. */
void eeprom_init(void);

/** @brief  Probe the device (write 0x55 to the last address, read it back). */
uint8_t eeprom_check(void);

uint8_t eeprom_read_one_byte(uint16_t addr);
void    eeprom_write_one_byte(uint16_t addr, uint8_t data);

void eeprom_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen);
void eeprom_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen);

#endif /* BSP_EEPROM_H */
