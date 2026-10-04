/**
 * @file    io_expand_pcf8574t.h
 * @brief   PCF8574T 8-bit I2C IO expander (chip driver).
 *
 * The device sits on the shared I2C bus (PH4/PH5). Its INT output is wired to
 * PB12 and is active low.
 */

#ifndef BSP_IO_EXPAND_PCF8574T_H
#define BSP_IO_EXPAND_PCF8574T_H

#include <stdint.h>
#include <stdbool.h>

/** @brief  Bring up the INT input and the IIC bus, then release all outputs.
 *  @return 0 if the expander acknowledged, 1 otherwise. */
uint8_t io_expand_pcf8574t_init(void);

uint8_t io_expand_pcf8574t_read(void);
void    io_expand_pcf8574t_write(uint8_t data);

/** @brief  True when the active-low INT output is asserted. */
bool    io_expand_pcf8574t_int_asserted(void);

#endif /* BSP_IO_EXPAND_PCF8574T_H */
