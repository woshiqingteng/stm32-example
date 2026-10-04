/**
 * @file    mag_st480mc.c
 * @brief   ST480MC magnetometer driver, ported from the vendor example. Uses
 *          the shared i2c bus driver (device id I2C_DEV_MAG, transaction API).
 */

#include <stddef.h>

#include "i2c.h"
#include "delay.h"
#include "mag_st480mc.h"

/* Command bytes. */
#define MAG_ST480MC_RESET        0xF0U   /* reset (RT) */
#define MAG_ST480MC_READ_DATA    0x4FU   /* read measurement, all (zyxt) */
#define MAG_ST480MC_SINGLE_MODE  0x3FU   /* start single measurement, all (zyxt) */

/* Single measurement needs ~20-30 ms to settle on this board; waiting only
 * 15 ms made the read-back report ERROR (status bit4) with 0xFF data. */
#define MAG_ST480MC_CONVERSION_MS  30U

static uint8_t mag_read_nbytes(uint8_t addr, uint8_t length, uint8_t *buf)
{
    return i2c_write_read(I2C_DEV_MAG, &addr, 1U, buf, (uint16_t)length) ? 0U : 1U;
}

uint8_t mag_st480mc_init(void)
{
    uint8_t status;
    uint8_t res    = 0xFFU;
    uint8_t retry  = 10U;

    i2c_init(NULL);

    /* Retry until the ST480MC answers with an ACK. */
    while ((retry-- != 0U) && (res != 0U))
    {
        res = mag_read_nbytes(MAG_ST480MC_RESET, 1U, &status);
        delay_ms(20U);
    }

    return res;
}

uint8_t mag_st480mc_read_magdata(int16_t *pmagx, int16_t *pmagy, int16_t *pmagz)
{
    uint8_t buf[7];

    (void)mag_read_nbytes((uint8_t)(MAG_ST480MC_SINGLE_MODE & 0xFEU), 1U, buf); /* single-shot, no temp */
    delay_ms(MAG_ST480MC_CONVERSION_MS);
    (void)mag_read_nbytes((uint8_t)(MAG_ST480MC_READ_DATA & 0xFEU), 7U, buf);   /* read mag data */

    if ((buf[0] & 0x10U) != 0U)
    {
        return buf[0];
    }

    *pmagx = (int16_t)(((int16_t)buf[1] << 8) | buf[2]);
    *pmagy = (int16_t)(((int16_t)buf[3] << 8) | buf[4]);
    *pmagz = (int16_t)(((int16_t)buf[5] << 8) | buf[6]);

    return 0U;
}

uint8_t mag_st480mc_read_temp(float *temp)
{
    uint8_t buf[9];
    uint16_t raw;

    (void)mag_read_nbytes(MAG_ST480MC_SINGLE_MODE, 1U, buf);   /* single-shot, with temp */
    delay_ms(MAG_ST480MC_CONVERSION_MS);
    (void)mag_read_nbytes(MAG_ST480MC_READ_DATA, 9U, buf);     /* read data + temp */

    if ((buf[0] & 0x10U) != 0U)
    {
        return buf[0];
    }

    raw   = (uint16_t)(((uint16_t)buf[1] << 8) | buf[2]);
    *temp = ((float)raw - 46244.0f) / 45.2f + 25.0f;          /* vendor transfer function */

    return 0U;
}
