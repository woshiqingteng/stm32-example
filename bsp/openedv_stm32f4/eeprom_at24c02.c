/**
 * @file    eeprom_at24c02.c
 * @brief   AT24C02 I2C EEPROM (chip driver): page-aware write, sequential
 *          read. Byte access is internal.
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
#define AT24C02_PAGE_SIZE      8U

/* ---- internal helpers ---- */

/* Send the device (write) address followed by the word address. */
static void at24c02_send_addr(uint16_t addr)
{
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
}

/* Random single-byte read (used by the device probe). */
static uint8_t at24c02_read_byte(uint16_t addr)
{
    uint8_t temp;

    i2c_start();
    at24c02_send_addr(addr);

    i2c_start();
    i2c_send_byte(0xA1U);
    i2c_wait_ack();
    temp = i2c_read_byte(0);
    i2c_stop();

    return temp;
}

/* Single-byte write. */
static void at24c02_write_byte(uint16_t addr, uint8_t data)
{
    i2c_start();
    at24c02_send_addr(addr);
    i2c_send_byte(data);
    i2c_wait_ack();
    i2c_stop();

    delay_ms(AT24C02_WRITE_DELAY_MS);
}

/* Page write: @p n <= AT24C02_PAGE_SIZE bytes, within one page. */
static void at24c02_page_write(uint16_t addr, const uint8_t *pbuf, uint8_t n)
{
    uint8_t i;

    i2c_start();
    at24c02_send_addr(addr);

    for (i = 0U; i < n; i++)
    {
        i2c_send_byte(pbuf[i]);
        i2c_wait_ack();
    }
    i2c_stop();

    delay_ms(AT24C02_WRITE_DELAY_MS);
}

/* ---- public API ---- */

void eeprom_at24c02_init(void)
{
    i2c_init();
}

/* Sequential read: the address auto-increments (wrapping at the 256-byte end). */
void eeprom_at24c02_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    uint16_t i;

    if (datalen == 0U)
    {
        return;
    }

    i2c_start();
    at24c02_send_addr(addr);

    i2c_start();
    i2c_send_byte(0xA1U);
    i2c_wait_ack();

    for (i = 0U; i < datalen; i++)
    {
        pbuf[i] = i2c_read_byte((i == (datalen - 1U)) ? 0U : 1U);
    }
    i2c_stop();
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
    uint16_t addr = EE_TYPE;

    temp = at24c02_read_byte(addr);

    if (temp == AT24C02_CHECK_VALUE)
    {
        return 0;
    }

    at24c02_write_byte(addr, AT24C02_CHECK_VALUE);
    temp = at24c02_read_byte(addr);

    return (temp == AT24C02_CHECK_VALUE) ? 0U : 1U;
}
