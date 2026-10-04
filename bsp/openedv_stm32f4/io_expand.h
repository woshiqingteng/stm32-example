/**
 * @file    io_expand.h
 * @brief   8-bit I2C IO expander: byte and bit access.
 *          Device-independent API over the chip driver (io_expand_pcf8574t.h).
 */

#ifndef BSP_IO_EXPAND_H
#define BSP_IO_EXPAND_H

#include <stdint.h>
#include <stdbool.h>

/* Expander port line assignment on the ALIENTEK F429 board. */
enum
{
    IO_EXPAND_BEEP = 0,     /*!< buzzer, active low (0 = sound, 1 = silent) */
    IO_EXPAND_AP_INT,       /*!< AP3216C interrupt (input)   */
    IO_EXPAND_DCMI_PWDN,    /*!< camera power-down, active low */
    IO_EXPAND_USB_PWR,      /*!< USB VBUS switch             */
    IO_EXPAND_EX,           /*!< EX_IO input                 */
    IO_EXPAND_MPU_INT,      /*!< IMU interrupt (input)       */
    IO_EXPAND_RS485_RE,     /*!< RS485 transceiver direction */
    IO_EXPAND_ETH_RESET     /*!< Ethernet PHY reset, active low */
};

/** @brief  Initialise the INT input and the IIC bus, then release all outputs.
 *  @return 0 if the expander acknowledged, 1 otherwise. */
uint8_t io_expand_init(void);

uint8_t io_expand_read(void);
void    io_expand_write(uint8_t data);

uint8_t io_expand_read_bit(uint8_t bit);
void    io_expand_write_bit(uint8_t bit, uint8_t sta);

/** @brief  True when the active-low INT output is asserted. */
bool    io_expand_int_asserted(void);

#endif /* BSP_IO_EXPAND_H */
