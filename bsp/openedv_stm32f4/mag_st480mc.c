/**
 * @file    mag_st480mc.c
 * @brief   ST480MC magnetometer driver, ported from the vendor example. Uses
 *          the shared i2c bus driver (device id I2C_DEV_MAG, transaction API).
 */

#include <stddef.h>

#include "i2c.h"
#include "delay.h"
#include "mag_st480mc.h"

/* Commands. RM/SM carry a "zyxt" nibble selecting the components to output:
 * bit3=Z, bit2=Y, bit1=X, bit0=T (all four = 0x3F / 0x4F). */
#define MAG_ST480MC_RESET        0xF0U   /* RT - reset (no status byte) */
#define MAG_ST480MC_SINGLE_MODE  0x3FU   /* SM - single measurement, zyxt=1111 */
#define MAG_ST480MC_READ_DATA    0x4FU   /* RM - read measurement,  zyxt=1111 */

/* Status byte (first byte of every response, RT excepted):
 *   [7] BURST_MODE  burst mode active
 *   [6] WOC_MODE    wake-up-on-change active
 *   [5] SM_MODE     single measurement mode active
 *   [4] ERROR       command rejected / uncorrectable ECC (only RT clears it)
 *   [3] SED         a single memory bit error was corrected (informative)
 *   [2] RS          set after a reset, cleared once the status byte is read
 *   [1:0] D         RM only: response data bytes = 2*D + 2 (2/4/6/8) */
#define MAG_ST480MC_ST_BURST_MODE  0x80U
#define MAG_ST480MC_ST_WOC_MODE    0x40U
#define MAG_ST480MC_ST_SM_MODE     0x20U
#define MAG_ST480MC_ST_ERROR       0x10U
#define MAG_ST480MC_ST_SED         0x08U
#define MAG_ST480MC_ST_RS          0x04U
#define MAG_ST480MC_ST_D_MASK      0x03U

/* RM response, read in order into buf[0..8] (fixed order, always T,X,Y,Z):
 *   buf[0]      status byte (bits above)
 *   buf[1..2]   Temp : 16-bit big-endian (high, low)
 *   buf[3..4]   X,  buf[5..6] Y,  buf[7..8] Z : same layout
 * Each component is two's complement; bit7 of the high byte is the sign. */
#define MAG_ST480MC_DATA_LEN       9U
#define MAG_ST480MC_TEMP_H         1U
#define MAG_ST480MC_TEMP_L         2U
#define MAG_ST480MC_X_H            3U
#define MAG_ST480MC_X_L            4U
#define MAG_ST480MC_Y_H            5U
#define MAG_ST480MC_Y_L            6U
#define MAG_ST480MC_Z_H            7U
#define MAG_ST480MC_Z_L            8U

/* Vendor temperature transfer function:
 *   T[C] = (raw - TEMP_RAW_REF) / TEMP_RAW_PER_C + TEMP_REF_C */
#define MAG_ST480MC_TEMP_RAW_REF    46244.0f   /* raw count at the reference */
#define MAG_ST480MC_TEMP_RAW_PER_C  45.2f      /* raw counts per degree Celsius */
#define MAG_ST480MC_TEMP_REF_C      25.0f      /* reference temperature (C) */

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

uint8_t mag_st480mc_read(int16_t *x, int16_t *y, int16_t *z, float *temp)
{
    uint8_t buf[MAG_ST480MC_DATA_LEN];
    uint16_t raw;

    (void)mag_read_nbytes(MAG_ST480MC_SINGLE_MODE, 1U, buf);   /* start measurement */
    delay_ms(MAG_ST480MC_CONVERSION_MS);
    (void)mag_read_nbytes(MAG_ST480MC_READ_DATA, MAG_ST480MC_DATA_LEN, buf);

    if ((buf[0] & MAG_ST480MC_ST_ERROR) != 0U)                 /* status ERROR */
    {
        return buf[0];
    }

    raw   = (uint16_t)(((uint16_t)buf[MAG_ST480MC_TEMP_H] << 8) | buf[MAG_ST480MC_TEMP_L]);
    *x    = (int16_t)(((int16_t)buf[MAG_ST480MC_X_H] << 8) | buf[MAG_ST480MC_X_L]);
    *y    = (int16_t)(((int16_t)buf[MAG_ST480MC_Y_H] << 8) | buf[MAG_ST480MC_Y_L]);
    *z    = (int16_t)(((int16_t)buf[MAG_ST480MC_Z_H] << 8) | buf[MAG_ST480MC_Z_L]);
    *temp = ((float)raw - MAG_ST480MC_TEMP_RAW_REF) / MAG_ST480MC_TEMP_RAW_PER_C
            + MAG_ST480MC_TEMP_REF_C;

    return 0U;
}
