/**
 * @file    io_expand_pcf8574t.c
 * @brief   PCF8574T 8-bit I2C IO expander (chip driver). All traffic goes
 *          through the shared I2C bus driver (device id I2C_DEV_IO_EXPAND).
 */

#include <stddef.h>

#include "io_expand_pcf8574t.h"
#include "gpio_hw.h"
#include "i2c.h"
#include "delay.h"

#define PCF8574T_WRITE_DELAY_MS 10U
/* All outputs released high. The buzzer (BEEP = P0) is active low, so the
 * released/pulled-up state keeps it silent. */
#define PCF8574T_IDLE_VALUE     0xFFU

/* Board facts for the PCF8574T: the I2C device and the INT input pin. */
typedef struct
{
    i2c_device_t dev;
    gpio_hw_t    int_gpio;
} io_expand_pcf8574t_hw_t;

static const io_expand_pcf8574t_hw_t g_hw =
{
    .dev = I2C_DEV_IO_EXPAND,
    .int_gpio = {
        .port      = GPIOB,
        .rcc_en    = RCC_AHB1ENR_GPIOBEN,
        .pin       = GPIO_PIN_12,
        .mode      = GPIO_MODE_INPUT,
        .pull      = GPIO_PULLUP,
        .speed     = GPIO_SPEED_FREQ_HIGH,
        .alternate = 0U,
    },
};

uint8_t io_expand_pcf8574t_init(void)
{
    uint8_t present;

    gpio_hw_setup(&g_hw.int_gpio);
    i2c_init(NULL);

    present = i2c_write(g_hw.dev, 0, 0U) ? 0U : 1U;   /* probe */
    io_expand_pcf8574t_write(PCF8574T_IDLE_VALUE);

    return present;
}

uint8_t io_expand_pcf8574t_read(void)
{
    uint8_t temp = 0U;

    (void)i2c_read(g_hw.dev, &temp, 1U);
    return temp;
}

void io_expand_pcf8574t_write(uint8_t data)
{
    (void)i2c_write(g_hw.dev, &data, 1U);
    delay_ms(PCF8574T_WRITE_DELAY_MS);
}

bool io_expand_pcf8574t_int_asserted(void)
{
    return (HAL_GPIO_ReadPin(g_hw.int_gpio.port, g_hw.int_gpio.pin) == GPIO_PIN_RESET) ? true : false;
}
