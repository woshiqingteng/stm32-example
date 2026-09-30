/**
 * @file    humi_dht11.c
 * @brief   DHT11 temperature / humidity sensor driver on PB12 (open drain).
 */

#include "stm32f4xx_hal.h"
#include "humi_dht11.h"
#include "delay.h"

#define HUMI_DHT11_DQ_HIGH()     HAL_GPIO_WritePin(HUMI_DHT11_DQ_GPIO_PORT, HUMI_DHT11_DQ_GPIO_PIN, GPIO_PIN_SET)
#define HUMI_DHT11_DQ_LOW()      HAL_GPIO_WritePin(HUMI_DHT11_DQ_GPIO_PORT, HUMI_DHT11_DQ_GPIO_PIN, GPIO_PIN_RESET)
#define HUMI_DHT11_DQ_READ()     HAL_GPIO_ReadPin(HUMI_DHT11_DQ_GPIO_PORT, HUMI_DHT11_DQ_GPIO_PIN)

#define HUMI_DHT11_RESPONSE_TIMEOUT_COUNT  100U
#define HUMI_DHT11_DATA_BYTE_COUNT        5U

static void humi_dht11_reset(void)
{
    HUMI_DHT11_DQ_LOW();
    delay_ms(20U);
    HUMI_DHT11_DQ_HIGH();
    delay_us(30U);
}

uint8_t humi_dht11_check(void)
{
    uint8_t retry = 0U;
    uint8_t rval  = 0U;

    while ((HUMI_DHT11_DQ_READ() != GPIO_PIN_RESET) && (retry < HUMI_DHT11_RESPONSE_TIMEOUT_COUNT))
    {
        retry++;
        delay_us(1U);
    }

    if (retry >= HUMI_DHT11_RESPONSE_TIMEOUT_COUNT)
    {
        rval = 1U;
    }
    else
    {
        retry = 0U;

        while ((HUMI_DHT11_DQ_READ() == GPIO_PIN_RESET) && (retry < HUMI_DHT11_RESPONSE_TIMEOUT_COUNT))
        {
            retry++;
            delay_us(1U);
        }

        if (retry >= HUMI_DHT11_RESPONSE_TIMEOUT_COUNT)
        {
            rval = 1U;
        }
    }

    return rval;
}

static uint8_t humi_dht11_read_bit(void)
{
    uint8_t retry = 0U;

    while ((HUMI_DHT11_DQ_READ() != GPIO_PIN_RESET) && (retry < HUMI_DHT11_RESPONSE_TIMEOUT_COUNT))
    {
        retry++;
        delay_us(1U);
    }

    retry = 0U;

    while ((HUMI_DHT11_DQ_READ() == GPIO_PIN_RESET) && (retry < HUMI_DHT11_RESPONSE_TIMEOUT_COUNT))
    {
        retry++;
        delay_us(1U);
    }

    delay_us(40U);

    return (HUMI_DHT11_DQ_READ() != GPIO_PIN_RESET) ? 1U : 0U;
}

static uint8_t humi_dht11_read_byte(void)
{
    uint8_t i;
    uint8_t data = 0U;

    for (i = 0U; i < 8U; i++)
    {
        data <<= 1;
        data |= humi_dht11_read_bit();
    }

    return data;
}

uint8_t humi_dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[HUMI_DHT11_DATA_BYTE_COUNT];
    uint8_t i;

    humi_dht11_reset();

    if (humi_dht11_check() != 0U)
    {
        return 1U;
    }

    for (i = 0U; i < HUMI_DHT11_DATA_BYTE_COUNT; i++)
    {
        buf[i] = humi_dht11_read_byte();
    }

    if (((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3])) != buf[4])
    {
        return 1U;
    }

    *humi = buf[0];
    *temp = buf[2];

    return 0U;
}

uint8_t humi_dht11_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: DQ clock + open-drain pin ---- */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin   = HUMI_DHT11_DQ_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HUMI_DHT11_DQ_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    humi_dht11_reset();

    return humi_dht11_check();
}
