/**
 * @file    pcf8574.c
 * @brief   PCF8574 8-bit I2C IO expander driver.
 */

#include "stm32f4xx_hal.h"
#include "iic.h"
#include "pcf8574.h"
#include "delay.h"

#define PCF8574_READ_ACK    0U
#define PCF8574_WRITE_DELAY_MS 10U
#define PCF8574_IDLE_VALUE  0xFFU

uint8_t pcf8574_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint8_t temp;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin   = PCF8574_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PCF8574_GPIO_PORT, &gpio_init);

    iic_init();

    iic_start();
    iic_send_byte(PCF8574_ADDR);
    temp = iic_wait_ack();
    iic_stop();

    pcf8574_write_byte(PCF8574_IDLE_VALUE);

    return temp;
}

uint8_t pcf8574_read_byte(void)
{
    uint8_t temp;

    iic_start();
    iic_send_byte(PCF8574_ADDR | 0x01U);
    iic_wait_ack();
    temp = iic_read_byte(PCF8574_READ_ACK);
    iic_stop();

    return temp;
}

bool pcf8574_int_asserted(void)
{
    return (PCF8574_INT == GPIO_PIN_RESET) ? true : false;
}

void pcf8574_write_byte(uint8_t data)
{
    iic_start();
    iic_send_byte(PCF8574_ADDR | 0x00U);
    iic_wait_ack();
    iic_send_byte(data);
    iic_wait_ack();
    iic_stop();

    delay_ms(PCF8574_WRITE_DELAY_MS);
}

void pcf8574_write_bit(uint8_t bit, uint8_t sta)
{
    uint8_t data = pcf8574_read_byte();

    if (sta == 0U)
    {
        data &= (uint8_t)~(1U << bit);
    }
    else
    {
        data |= (uint8_t)(1U << bit);
    }

    pcf8574_write_byte(data);
}

uint8_t pcf8574_read_bit(uint8_t bit)
{
    uint8_t data = pcf8574_read_byte();

    return (uint8_t)((data >> bit) & 0x01U);
}
