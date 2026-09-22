/**
 * @file    iic.c
 * @brief   Software (bit-bang) IIC master on PH4 (SCL) / PH5 (SDA).
 *
 * Timing matches the vendor driver: each half period is ~2us, i.e. roughly
 * 250kHz. SDA is open-drain so writing 1 releases the line and reading it
 * returns the external level.
 */

#include "stm32f4xx_hal.h"
#include "iic.h"
#include "delay.h"

#define IIC_DELAY_US      2U
#define IIC_ACK_TIMEOUT   250U

#define IIC_SCL(x)  do { (x) ? HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define IIC_SDA(x)  do { (x) ? HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define IIC_READ_SDA  HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN)

static void iic_delay(void)
{
    delay_us(IIC_DELAY_US);
}

void iic_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio_init.Pin   = IIC_SCL_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(IIC_SCL_GPIO_PORT, &gpio_init);

    gpio_init.Pin  = IIC_SDA_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(IIC_SDA_GPIO_PORT, &gpio_init);

    iic_stop();
}

void iic_start(void)
{
    IIC_SDA(1);
    IIC_SCL(1);
    iic_delay();
    IIC_SDA(0);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
}

void iic_stop(void)
{
    IIC_SDA(0);
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SDA(1);
    iic_delay();
}

uint8_t iic_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    IIC_SDA(1);
    iic_delay();
    IIC_SCL(1);
    iic_delay();

    while (IIC_READ_SDA)
    {
        waittime++;

        if (waittime > IIC_ACK_TIMEOUT)
        {
            iic_stop();
            rack = 1;
            break;
        }

        iic_delay();
    }

    IIC_SCL(0);
    iic_delay();

    return rack;
}

void iic_ack(void)
{
    IIC_SDA(0);
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
    IIC_SDA(1);
    iic_delay();
}

void iic_nack(void)
{
    IIC_SDA(1);
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
}

void iic_send_byte(uint8_t data)
{
    uint8_t t;

    for (t = 0; t < 8U; t++)
    {
        IIC_SDA((data & 0x80U) >> 7);
        iic_delay();
        IIC_SCL(1);
        iic_delay();
        IIC_SCL(0);
        data <<= 1;
    }

    IIC_SDA(1);
}

uint8_t iic_read_byte(uint8_t ack)
{
    uint8_t i;
    uint8_t receive = 0;

    for (i = 0; i < 8U; i++)
    {
        receive <<= 1;
        IIC_SCL(1);
        iic_delay();

        if (IIC_READ_SDA != 0U)
        {
            receive++;
        }

        IIC_SCL(0);
        iic_delay();
    }

    if (ack != 0U)
    {
        iic_ack();
    }
    else
    {
        iic_nack();
    }

    return receive;
}
