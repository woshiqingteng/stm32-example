/**
 * @file    eeprom_at24c02.h
 * @brief   AT24C02 I2C EEPROM (chip driver): block read/write.
 *
 * The address encoding also covers the wider AT24Cxx series. Byte access is
 * internal; the public API is block read/write only.
 */

#ifndef BSP_EEPROM_AT24C02_H
#define BSP_EEPROM_AT24C02_H

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

/** @brief  Bring up the shared software IIC bus. */
void eeprom_at24c02_init(void);

/** @brief  Probe the device (write 0x55 to the last address, read it back). */
uint8_t eeprom_at24c02_check(void);

/** @brief  Read @p datalen bytes starting at @p addr (sequential read). */
void eeprom_at24c02_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen);

/** @brief  Write @p datalen bytes starting at @p addr (page-aware). */
void eeprom_at24c02_write(uint16_t addr, const uint8_t *pbuf, uint16_t datalen);

#endif /* BSP_EEPROM_AT24C02_H */
