/**
 * @file    eeprom_at24c02.c
 * @brief   AT24C02 I2C EEPROM (chip driver): page-aware write, sequential
 *          read. Byte access is internal. All traffic goes through the shared
 *          I2C bus driver (device id I2C_DEV_EEPROM, 7-bit address 0x50).
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "eeprom_at24c02.h"
#include "delay.h"

#define AT24C02_SIZE_BYTE      256U
#define AT24C02_WRITE_DELAY_MS 10U
#define AT24C02_CHECK_VALUE    0x55U
#define AT24C02_PAGE_SIZE      8U

/* Word address is 8-bit on the AT24C02. */
#define AT24C02_WORD(a)        ((uint8_t)((a) & 0xFFU))

/* ---- internal helpers ---- */

/* Random single-byte read (used by the device probe). */
static uint8_t at24c02_read_byte(uint16_t addr)
{
    uint8_t word = AT24C02_WORD(addr);
    uint8_t data = 0U;

    (void)i2c_write_read(I2C_DEV_EEPROM, &word, 1U, &data, 1U);
    return data;
}

/* Single-byte write. */
static void at24c02_write_byte(uint16_t addr, uint8_t data)
{
    uint8_t buf[2];

    buf[0] = AT24C02_WORD(addr);
    buf[1] = data;
    (void)i2c_write(I2C_DEV_EEPROM, buf, 2U);
    delay_ms(AT24C02_WRITE_DELAY_MS);
}

/* Page write: @p n <= AT24C02_PAGE_SIZE bytes, within one page. */
static void at24c02_page_write(uint16_t addr, const uint8_t *pbuf, uint8_t n)
{
    uint8_t buf[AT24C02_PAGE_SIZE + 1U];
    uint8_t i;

    buf[0] = AT24C02_WORD(addr);
    for (i = 0U; i < n; i++)
    {
        buf[i + 1U] = pbuf[i];
    }
    (void)i2c_write(I2C_DEV_EEPROM, buf, (uint16_t)(n + 1U));
    delay_ms(AT24C02_WRITE_DELAY_MS);
}

/* ---- public API ---- */

void eeprom_at24c02_init(void)
{
    i2c_init(0);
}

/* Sequential read: the address auto-increments (wrapping at the 256-byte end). */
void eeprom_at24c02_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    uint8_t word;

    if (datalen == 0U)
    {
        return;
    }
    word = AT24C02_WORD(addr);
    (void)i2c_write_read(I2C_DEV_EEPROM, &word, 1U, pbuf, datalen);
}

/* Block write: align with byte writes, whole pages as page writes, tail bytes. */
void eeprom_at24c02_write(uint16_t addr, const uint8_t *pbuf, uint16_t datalen)
{
    while ((datalen != 0U) && ((addr % AT24C02_PAGE_SIZE) != 0U))
    {
        at24c02_write_byte(addr, *pbuf);
        addr++;
        pbuf++;
        datalen--;
    }

    while (datalen >= AT24C02_PAGE_SIZE)
    {
        at24c02_page_write(addr, pbuf, AT24C02_PAGE_SIZE);
        addr    += AT24C02_PAGE_SIZE;
        pbuf    += AT24C02_PAGE_SIZE;
        datalen -= AT24C02_PAGE_SIZE;
    }

    while (datalen != 0U)
    {
        at24c02_write_byte(addr, *pbuf);
        addr++;
        pbuf++;
        datalen--;
    }
}

uint8_t eeprom_at24c02_check(void)
{
    uint8_t temp;
    uint16_t addr = AT24C02_SIZE_BYTE - 1U;

    temp = at24c02_read_byte(addr);

    if (temp == AT24C02_CHECK_VALUE)
    {
        return 0;
    }

    at24c02_write_byte(addr, AT24C02_CHECK_VALUE);
    temp = at24c02_read_byte(addr);

    return (temp == AT24C02_CHECK_VALUE) ? 0U : 1U;
}
