/**
 * @file    i2c.h
 * @brief   Shared IIC master bus of the ALIENTEK F429 board (SCL = PH4,
 *          SDA = PH5). Two backends are selectable at run time: a software
 *          bit-bang master (default) or the hardware I2C2 peripheral; the
 *          hardware transfers are polled / interrupt / DMA driven.
 *          This interface is HAL-free (own enums).
 *
 * The bus enumerates the on-board I2C slaves; every transaction only deals
 * with a device id and data (the 7-bit addresses live in i2c.c).
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>
#include <stdbool.h>

/** @brief I2C slave device selector (7-bit address lives in i2c.c). */
typedef enum
{
    I2C_DEV_EEPROM = 0, /*!< AT24C02   0x50 */
    I2C_DEV_CODEC,      /*!< ES8388    0x10 */
    I2C_DEV_ALS,        /*!< AP3216C   0x1E */
    I2C_DEV_IMU,        /*!< SH3001    0x36 */
    I2C_DEV_MAG,        /*!< ST480MC   0x0C */
    I2C_DEV_IO_EXPAND,  /*!< PCF8574   0x20 */
    I2C_DEV_NUM
} i2c_device_t;

/** @brief Backend. */
typedef enum
{
    I2C_BACKEND_SW = 0, /*!< software bit-bang master (default) */
    I2C_BACKEND_HW      /*!< hardware I2C2 peripheral (PH4/PH5, AF4) */
} i2c_backend_t;

/** @brief Hardware transfer transport (ignored by the SW backend). */
typedef enum
{
    I2C_XFER_POLL = 0, /*!< CPU polled byte-by-byte */
    I2C_XFER_IT,       /*!< interrupt driven */
    I2C_XFER_DMA       /*!< DMA driven (read and write) */
} i2c_xfer_t;

/** @brief Bus configuration. */
typedef struct
{
    i2c_backend_t backend;  /*!< bit-bang or hardware peripheral */
    i2c_xfer_t    xfer;     /*!< hardware transport (ignored for the SW backend) */
    uint32_t      speed_hz; /*!< target bus speed (SW derives its half-period from it) */
} i2c_cfg_t;

/** @brief Software bit-bang at 100 kHz (safe for every device on the bus). */
#define I2C_CFG_DEFAULT \
    .backend = I2C_BACKEND_SW, .xfer = I2C_XFER_DMA, .speed_hz = 100000U

/**
 * @brief  Initialise the bus according to @p cfg (NULL selects I2C_CFG_DEFAULT).
 *         A NULL cfg after the first initialisation is a no-op, so every device
 *         driver may call i2c_init(NULL); an application can call
 *         i2c_init(&cfg) first to select the hardware backend.
 */
void i2c_init(const i2c_cfg_t *cfg);

/**
 * @brief  Master transmit @p len bytes to @p dev.
 * @param  buf data bytes (may be NULL when @p len is 0).
 * @param  len set 0 to only probe the device address (START/address/STOP).
 * @return true on success (all bytes acknowledged).
 */
bool i2c_write(i2c_device_t dev, const uint8_t *buf, uint16_t len);

/** @brief  Master receive @p len bytes from @p dev. true on success. */
bool i2c_read(i2c_device_t dev, uint8_t *buf, uint16_t len);

/**
 * @brief  Write @p wlen bytes then (repeated start) read @p rlen bytes.
 * @param  wlen must be >= 1 (use i2c_read for a plain read).
 * @return true on success.
 */
bool i2c_write_read(i2c_device_t dev,
                    const uint8_t *wbuf, uint16_t wlen,
                    uint8_t *rbuf, uint16_t rlen);

#endif /* BSP_I2C_H */
