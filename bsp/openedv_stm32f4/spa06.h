/**
 * @file    spa06.h
 * @brief   SPA06 barometric pressure / temperature sensor driver.
 */

#ifndef BSP_SPA06_H
#define BSP_SPA06_H

#include <stdint.h>

/** @brief  7-bit I2C address (SDO = 0). */
#define SPA06_I2C_ADDR                  0x76U

#define SPA06_DEFAULT_CHIP_ID           0x11U

/* Register map. */
#define SPA06_PRESSURE_MSB_REG          0x00U
#define SPA06_TEMPERATURE_MSB_REG       0x03U
#define SPA06_PRESSURE_CFG_REG          0x06U
#define SPA06_TEMPERATURE_CFG_REG       0x07U
#define SPA06_MODE_CFG_REG              0x08U
#define SPA06_INT_FIFO_CFG_REG          0x09U
#define SPA06_CHIP_ID                   0x0DU
#define SPA06_COEFFICIENT_CALIB_REG     0x10U

#define SPA06_CALIB_COEFFICIENT_LENGTH  18U
#define SPA06_DATA_FRAME_SIZE           6U
#define SPA06_CONTINUOUS_MODE           0x07U

#define SPA06_MEASURE_16                0x04U
#define SPA06_OVERSAMP_8                0x03U
#define SPA06_OVERSAMP_64               0x06U

/** @brief  Compensated measurement result. */
typedef struct
{
    int32_t praw;   /*!< raw pressure count */
    int32_t traw;   /*!< raw temperature count */
    float   pcomp;  /*!< compensated pressure in hPa */
    float   tcomp;  /*!< compensated temperature in degrees Celsius */
} spa06_result_t;

/** @brief  Reset/probe the sensor and start continuous acquisition.
 *  @return 0 on success, 1 on a chip-id mismatch. */
uint8_t spa06_init(void);

/** @brief  Read and compensate the latest pressure and temperature sample. */
void spa06_get_data(spa06_result_t *p_res);

uint8_t spa06_read_byte(uint8_t reg);
uint8_t spa06_write_byte(uint8_t reg, uint8_t data);

#endif /* BSP_SPA06_H */
