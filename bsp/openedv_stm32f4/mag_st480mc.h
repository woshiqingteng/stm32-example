/**
 * @file    mag_st480mc.h
 * @brief   ST480MC magnetometer (3-axis + temperature) over the shared IIC
 *          bus (SCL = PH4, SDA = PH5), ported from the vendor example.
 */

#ifndef BSP_MAG_ST480MC_H
#define BSP_MAG_ST480MC_H

#include <stdint.h>

#define MAG_ST480MC_RESET           0xF0U   /* reset command */
#define MAG_ST480MC_READ_DATA       0x4FU   /* read all data (zxyt) */
#define MAG_ST480MC_SINGLE_MODE     0x3FU   /* single-shot read (zxyt) */

/** @brief  Reset the ST480MC and probe its IIC address. 0 on success. */
uint8_t mag_st480mc_init(void);

/** @brief  Read one raw magnetic sample (single-shot). 0 on success. */
uint8_t mag_st480mc_read_magdata(int16_t *pmagx, int16_t *pmagy, int16_t *pmagz);

/** @brief  Read the temperature (Celsius, single-shot). 0 on success. */
uint8_t mag_st480mc_read_temperature(float *ptemp);

/** @brief  Average @p times single-shot magnetic samples. 0 on success. */
uint8_t mag_st480mc_read_magdata_average(int16_t *pmagx, int16_t *pmagy, int16_t *pmagz, uint8_t times);

#endif /* BSP_MAG_ST480MC_H */
