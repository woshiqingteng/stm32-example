/**
 * @file    ds18b20.c
 * @brief   DS18B20 1-Wire temperature sensor driver on PB12 (open drain).
 */

#include "stm32f4xx_hal.h"
#include "ds18b20.h"
#include "delay.h"

#define DS18B20_CMD_SKIP_ROM   0xCCU
#define DS18B20_CMD_CONVERT    0x44U
#define DS18B20_CMD_READ       0xBEU

#define DS18B20_DQ_HIGH()      HAL_GPIO_WritePin(DS18B20_DQ_GPIO_PORT, DS18B20_DQ_GPIO_PIN, GPIO_PIN_SET)
#define DS18B20_DQ_LOW()       HAL_GPIO_WritePin(DS18B20_DQ_GPIO_PORT, DS18B20_DQ_GPIO_PIN, GPIO_PIN_RESET)
#define DS18B20_DQ_READ()      HAL_GPIO_ReadPin(DS18B20_DQ_GPIO_PORT, DS18B20_DQ_GPIO_PIN)

static void ds18b20_reset(void)
{
    DS18B20_DQ_LOW();
    delay_us(750U);
    DS18B20_DQ_HIGH();
    delay_us(15U);
}

uint8_t ds18b20_check(void)
{
    uint8_t retry = 0U;
    uint8_t rval  = 0U;

    while ((DS18B20_DQ_READ() != GPIO_PIN_RESET) && (retry < 200U))
    {
        retry++;
        delay_us(1U);
    }

    if (retry >= 200U)
    {
        rval = 1U;
    }
    else
    {
        retry = 0U;

        while ((DS18B20_DQ_READ() == GPIO_PIN_RESET) && (retry < 240U))
        {
            retry++;
            delay_us(1U);
        }

        if (retry >= 240U)
        {
            rval = 1U;
        }
    }

    return rval;
}

static uint8_t ds18b20_read_bit(void)
{
    uint8_t data = 0U;

    DS18B20_DQ_LOW();
    delay_us(2U);
    DS18B20_DQ_HIGH();
    delay_us(12U);

    if (DS18B20_DQ_READ() != GPIO_PIN_RESET)
    {
        data = 1U;
    }

    delay_us(50U);

    return data;
}

static uint8_t ds18b20_read_byte(void)
{
    uint8_t i;
    uint8_t data = 0U;

    for (i = 0U; i < 8U; i++)
    {
        data |= (uint8_t)(ds18b20_read_bit() << i);
    }

    return data;
}

static void ds18b20_write_byte(uint8_t data)
{
    uint8_t j;

    for (j = 0U; j < 8U; j++)
    {
        if ((data & 0x01U) != 0U)
        {
            DS18B20_DQ_LOW();
            delay_us(2U);
            DS18B20_DQ_HIGH();
            delay_us(60U);
        }
        else
        {
            DS18B20_DQ_LOW();
            delay_us(60U);
            DS18B20_DQ_HIGH();
            delay_us(2U);
        }

        data >>= 1;
    }
}

static void ds18b20_start(void)
{
    ds18b20_reset();
    (void)ds18b20_check();
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_CONVERT);
}

uint8_t ds18b20_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: DQ clock + open-drain pin ---- */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin   = DS18B20_DQ_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_DQ_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    ds18b20_reset();

    return ds18b20_check();
}

int16_t ds18b20_get_temperature(void)
{
    uint8_t  negative = 0U;
    uint8_t  tl;
    uint8_t  th;
    uint16_t raw;
    int16_t  temp;

    ds18b20_start();
    ds18b20_reset();
    (void)ds18b20_check();
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_READ);

    tl = ds18b20_read_byte();
    th = ds18b20_read_byte();

    if (th > 7U) /* negative temperature */
    {
        th = (uint8_t)~th;
        tl = (uint8_t)~tl;
        negative = 1U;
    }

    raw  = (uint16_t)th;
    raw <<= 8;
    raw += tl;

    /* raw is in 1/16 degree units; convert to tenths of a degree. */
    if (negative != 0U)
    {
        temp = (int16_t)(-(((int32_t)raw + 1) * 5 / 8));
    }
    else
    {
        temp = (int16_t)(((int32_t)raw * 5) / 8);
    }

    return temp;
}
