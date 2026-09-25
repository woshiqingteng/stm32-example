/**
 * @file    i2c.c
 * @brief   Software (bit-bang) IIC master on PH4 (SCL) / PH5 (SDA).
 *
 * Timing matches the vendor driver: each half period is ~2us, i.e. roughly
 * 250kHz. SDA is open-drain so writing 1 releases the line and reading it
 * returns the external level.
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "delay.h"

#define I2C_DELAY_US      2U
#define I2C_ACK_TIMEOUT   250U

#define I2C_SCL(x)  do { (x) ? HAL_GPIO_WritePin(I2C_SCL_GPIO_PORT, I2C_SCL_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(I2C_SCL_GPIO_PORT, I2C_SCL_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define I2C_SDA(x)  do { (x) ? HAL_GPIO_WritePin(I2C_SDA_GPIO_PORT, I2C_SDA_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(I2C_SDA_GPIO_PORT, I2C_SDA_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define I2C_READ_SDA  HAL_GPIO_ReadPin(I2C_SDA_GPIO_PORT, I2C_SDA_GPIO_PIN)

static void i2c_delay(void)
{
    delay_us(I2C_DELAY_US);
}

void i2c_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio_init.Pin   = I2C_SCL_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(I2C_SCL_GPIO_PORT, &gpio_init);

    gpio_init.Pin  = I2C_SDA_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(I2C_SDA_GPIO_PORT, &gpio_init);

    i2c_stop();
}

void i2c_start(void)
{
    I2C_SDA(1);
    I2C_SCL(1);
    i2c_delay();
    I2C_SDA(0);
    i2c_delay();
    I2C_SCL(0);
    i2c_delay();
}

void i2c_stop(void)
{
    I2C_SDA(0);
    i2c_delay();
    I2C_SCL(1);
    i2c_delay();
    I2C_SDA(1);
    i2c_delay();
}

uint8_t i2c_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    I2C_SDA(1);
    i2c_delay();
    I2C_SCL(1);
    i2c_delay();

    while (I2C_READ_SDA)
    {
        waittime++;

        if (waittime > I2C_ACK_TIMEOUT)
        {
            i2c_stop();
            rack = 1;
            break;
        }

        i2c_delay();
    }

    I2C_SCL(0);
    i2c_delay();

    return rack;
}

void i2c_ack(void)
{
    I2C_SDA(0);
    i2c_delay();
    I2C_SCL(1);
    i2c_delay();
    I2C_SCL(0);
    i2c_delay();
    I2C_SDA(1);
    i2c_delay();
}

void i2c_nack(void)
{
    I2C_SDA(1);
    i2c_delay();
    I2C_SCL(1);
    i2c_delay();
    I2C_SCL(0);
    i2c_delay();
}

void i2c_send_byte(uint8_t data)
{
    uint8_t t;

    for (t = 0; t < 8U; t++)
    {
        I2C_SDA((data & 0x80U) >> 7);
        i2c_delay();
        I2C_SCL(1);
        i2c_delay();
        I2C_SCL(0);
        data <<= 1;
    }

    I2C_SDA(1);
}

uint8_t i2c_read_byte(uint8_t ack)
{
    uint8_t i;
    uint8_t receive = 0;

    for (i = 0; i < 8U; i++)
    {
        receive <<= 1;
        I2C_SCL(1);
        i2c_delay();

        if (I2C_READ_SDA != 0U)
        {
            receive++;
        }

        I2C_SCL(0);
        i2c_delay();
    }

    if (ack != 0U)
    {
        i2c_ack();
    }
    else
    {
        i2c_nack();
    }

    return receive;
}
