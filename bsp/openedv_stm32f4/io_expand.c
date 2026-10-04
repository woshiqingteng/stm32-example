/**
 * @file    io_expand.c
 * @brief   PCF8574 8-bit I2C IO expander driver.
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "io_expand.h"
#include "delay.h"

#define PCF8574_WRITE_DELAY_MS 10U
#define PCF8574_IDLE_VALUE  0xFFU

uint8_t io_expand_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint8_t temp;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin   = PCF8574_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PCF8574_GPIO_PORT, &gpio_init);

    i2c_init(0);

    temp = i2c_write(I2C_DEV_IO_EXPAND, 0, 0U) ? 0U : 1U;   /* probe */

    io_expand_write_byte(PCF8574_IDLE_VALUE);

    return temp;
}

uint8_t io_expand_read_byte(void)
{
    uint8_t temp = 0U;

    (void)i2c_read(I2C_DEV_IO_EXPAND, &temp, 1U);
    return temp;
}

bool io_expand_int_asserted(void)
{
    return (PCF8574_INT == GPIO_PIN_RESET) ? true : false;
}

void io_expand_write_byte(uint8_t data)
{
    (void)i2c_write(I2C_DEV_IO_EXPAND, &data, 1U);
    delay_ms(PCF8574_WRITE_DELAY_MS);
}

void io_expand_write_bit(uint8_t bit, uint8_t sta)
{
    uint8_t data = io_expand_read_byte();

    if (sta == 0U)
    {
        data &= (uint8_t)~(1U << bit);
    }
    else
    {
        data |= (uint8_t)(1U << bit);
    }

    io_expand_write_byte(data);
}

uint8_t io_expand_read_bit(uint8_t bit)
{
    uint8_t data = io_expand_read_byte();

    return (uint8_t)((data >> bit) & 0x01U);
}
