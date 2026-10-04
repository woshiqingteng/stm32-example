/**
 * @file    i2c.h
 * @brief   Shared IIC master bus of the ALIENTEK F429 board (SCL = PH4,
 *          SDA = PH5). One runtime selector (i2c_io_t) chooses either the
 *          software bit-bang master (default) or a hardware I2C2 transport
 *          (polled / interrupt / DMA). This interface is HAL-free (own enums).
 *
 * The bus enumerates the on-board I2C slaves; every transaction only deals
 * with a device id and data (the 7-bit addresses live in i2c.c).
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

/** @brief Transfer transport (mirrors usart_io_t): one runtime path selects
 *         either the software bit-bang master or a hardware I2C2 transport. */
typedef enum
{
    I2C_IO_SW = 0, /*!< software bit-bang master, fixed 100 kHz */
    I2C_IO_POLL,   /*!< hardware I2C2, CPU polled */
    I2C_IO_IT,     /*!< hardware I2C2, interrupt driven */
    I2C_IO_DMA     /*!< hardware I2C2, DMA driven */
} i2c_io_t;

/** @brief Bus instance selector (one bus today; the id leaves room for more,
 *         mirroring usart_id_t). Selected by i2c_init, not by the runtime API. */
typedef enum
{
    I2C_ID_1 = 0, /*!< I2C2: PH4 SCL / PH5 SDA (shared by all on-board slaves) */
    I2C_ID_NUM
} i2c_id_t;

/** @brief Init frame options (HAL-free mirrors of the HAL I2C Init enums). */
typedef enum { I2C_DUTY_2 = 0, I2C_DUTY_16_9 }                  i2c_duty_t;
typedef enum { I2C_ADDR_7BIT = 0, I2C_ADDR_10BIT }             i2c_addr_mode_t;
typedef enum { I2C_DUAL_DISABLE = 0, I2C_DUAL_ENABLE }         i2c_dual_addr_t;
typedef enum { I2C_GCALL_DISABLE = 0, I2C_GCALL_ENABLE }       i2c_general_call_t;
typedef enum { I2C_STRETCH_ENABLE = 0, I2C_STRETCH_DISABLE }   i2c_stretch_t;

/** @brief Bus configuration (settable): transport + the I2C Init frame options. */
typedef struct
{
    i2c_id_t id;            /*!< bus instance */
    i2c_io_t io;            /*!< transfer transport */
    uint32_t speed_hz;      /*!< -> Init.ClockSpeed (SW backend is fixed 100 kHz) */

    i2c_duty_t         duty_cycle;         /*!< -> Init.DutyCycle       */
    uint16_t           own_address1;       /*!< -> Init.OwnAddress1     */
    i2c_addr_mode_t    addressing_mode;    /*!< -> Init.AddressingMode  */
    i2c_dual_addr_t    dual_address_mode;  /*!< -> Init.DualAddressMode */
    uint8_t            own_address2;       /*!< -> Init.OwnAddress2     */
    i2c_general_call_t general_call_mode;  /*!< -> Init.GeneralCallMode */
    i2c_stretch_t      stretch;            /*!< -> Init.NoStretchMode   */

    uint32_t irq_preempt;   /*!< NVIC preemption priority */
    uint32_t irq_sub;       /*!< NVIC subpriority */
} i2c_cfg_t;

/** @brief Software bit-bang at 100 kHz (safe for every device on the bus). */
#define I2C_CFG_DEFAULT(inst) \
    .id = (inst), .io = I2C_IO_SW, .speed_hz = 100000U, \
    .duty_cycle = I2C_DUTY_2, .own_address1 = 0U, \
    .addressing_mode = I2C_ADDR_7BIT, .dual_address_mode = I2C_DUAL_DISABLE, \
    .own_address2 = 0U, .general_call_mode = I2C_GCALL_DISABLE, \
    .stretch = I2C_STRETCH_ENABLE, \
    .irq_preempt = 3U, .irq_sub = 3U

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
