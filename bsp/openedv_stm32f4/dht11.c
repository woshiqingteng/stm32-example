/**
 * @file    dht11.c
 * @brief   DHT11 temperature / humidity sensor driver on PB12 (open drain).
 */

#include "stm32f4xx_hal.h"
#include "dht11.h"
#include "delay.h"

#define DHT11_DQ_HIGH()     HAL_GPIO_WritePin(DHT11_DQ_GPIO_PORT, DHT11_DQ_GPIO_PIN, GPIO_PIN_SET)
#define DHT11_DQ_LOW()      HAL_GPIO_WritePin(DHT11_DQ_GPIO_PORT, DHT11_DQ_GPIO_PIN, GPIO_PIN_RESET)
#define DHT11_DQ_READ()     HAL_GPIO_ReadPin(DHT11_DQ_GPIO_PORT, DHT11_DQ_GPIO_PIN)

#define DHT11_RESPONSE_TIMEOUT  100U
#define DHT11_DATA_BYTES        5U

static void dht11_reset(void)
{
    DHT11_DQ_LOW();
    delay_ms(20U);
    DHT11_DQ_HIGH();
    delay_us(30U);
}

uint8_t dht11_check(void)
{
    uint8_t retry = 0U;
    uint8_t rval  = 0U;

    while ((DHT11_DQ_READ() != GPIO_PIN_RESET) && (retry < DHT11_RESPONSE_TIMEOUT))
    {
        retry++;
        delay_us(1U);
    }

    if (retry >= DHT11_RESPONSE_TIMEOUT)
    {
        rval = 1U;
    }
    else
    {
        retry = 0U;

        while ((DHT11_DQ_READ() == GPIO_PIN_RESET) && (retry < DHT11_RESPONSE_TIMEOUT))
        {
            retry++;
            delay_us(1U);
        }

        if (retry >= DHT11_RESPONSE_TIMEOUT)
        {
            rval = 1U;
        }
    }

    return rval;
}

static uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0U;

    while ((DHT11_DQ_READ() != GPIO_PIN_RESET) && (retry < DHT11_RESPONSE_TIMEOUT))
    {
        retry++;
        delay_us(1U);
    }

    retry = 0U;

    while ((DHT11_DQ_READ() == GPIO_PIN_RESET) && (retry < DHT11_RESPONSE_TIMEOUT))
    {
        retry++;
        delay_us(1U);
    }

    delay_us(40U);

    return (DHT11_DQ_READ() != GPIO_PIN_RESET) ? 1U : 0U;
}

static uint8_t dht11_read_byte(void)
{
    uint8_t i;
    uint8_t data = 0U;

    for (i = 0U; i < 8U; i++)
    {
        data <<= 1;
        data |= dht11_read_bit();
    }

    return data;
}

uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[DHT11_DATA_BYTES];
    uint8_t i;

    dht11_reset();

    if (dht11_check() != 0U)
    {
        return 1U;
    }

    for (i = 0U; i < DHT11_DATA_BYTES; i++)
    {
        buf[i] = dht11_read_byte();
    }

    if (((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3])) != buf[4])
    {
        return 1U;
    }

    *humi = buf[0];
    *temp = buf[2];

    return 0U;
}

uint8_t dht11_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: DQ clock + open-drain pin ---- */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin   = DHT11_DQ_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DQ_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    dht11_reset();

    return dht11_check();
}
