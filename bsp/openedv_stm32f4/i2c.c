/**
 * @file    i2c.c
 * @brief   IIC master on PH4 (SCL) / PH5 (SDA). The backend is chosen at compile
 *          time with BSP_I2C_USE_HARDWARE: a software bit-bang master (default,
 *          each half period ~2us, roughly 250kHz) or the hardware I2C2
 *          peripheral (PH4/PH5, AF4, BSP_I2C_SPEED_HZ). Both implement the same
 *          primitive API.
 */

#include "stm32f4xx_hal.h"
#include "i2c.h"

#if BSP_I2C_USE_HARDWARE

/* ===================== hardware I2C2 backend ===================== */

#define I2C_HW_TIMEOUT   0x000FFFFFU

static I2C_HandleTypeDef g_i2c_handle;
static uint8_t           g_i2c_addr_phase;

/**
 * @brief  Wait until SR1 has any of the two flag bits set (f2 = 0 waits f1 only).
 * @return 1 on timeout, 0 otherwise.
 */
static uint8_t i2c_hw_wait_any(uint32_t f1, uint32_t f2)
{
    uint32_t time = I2C_HW_TIMEOUT;

    while (((I2C2->SR1 & f1) == 0U) && ((I2C2->SR1 & f2) == 0U))
    {
        if (time-- == 0U)
        {
            return 1U;
        }
    }

    return 0U;
}

void i2c_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    gpio_init.Pin       = I2C_SCL_GPIO_PIN | I2C_SDA_GPIO_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_OD;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(I2C_SCL_GPIO_PORT, &gpio_init);

    g_i2c_handle.Instance             = I2C2;
    g_i2c_handle.Init.ClockSpeed      = BSP_I2C_SPEED_HZ;
    g_i2c_handle.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    g_i2c_handle.Init.OwnAddress1     = 0U;
    g_i2c_handle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    g_i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    g_i2c_handle.Init.OwnAddress2     = 0U;
    g_i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    g_i2c_handle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    (void)HAL_I2C_Init(&g_i2c_handle);

    g_i2c_addr_phase = 0U;
    I2C2->CR1 |= I2C_CR1_STOP;               /* release the bus */
}

void i2c_start(void)
{
    I2C2->CR1 |= I2C_CR1_START;
    (void)i2c_hw_wait_any(I2C_SR1_SB, 0U);   /* start condition sent */
    I2C2->CR1 |= I2C_CR1_ACK;                /* ACK the first received byte */
    g_i2c_addr_phase = 1U;
}

void i2c_stop(void)
{
    I2C2->CR1 &= ~I2C_CR1_ACK;
    I2C2->CR1 |= I2C_CR1_STOP;
    __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_SR1_AF);   /* clear a pending NACK */
}

void i2c_send_byte(uint8_t data)
{
    (void)i2c_hw_wait_any(I2C_SR1_TXE, 0U);
    I2C2->DR = data;
}

uint8_t i2c_wait_ack(void)
{
    if (g_i2c_addr_phase != 0U)
    {
        /* address phase ends with ADDR (ACK) or AF (NACK) */
        (void)i2c_hw_wait_any(I2C_SR1_ADDR, I2C_SR1_AF);
        g_i2c_addr_phase = 0U;

        if ((I2C2->SR1 & I2C_SR1_AF) != 0U)
        {
            __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_SR1_AF);
            return 1U;
        }

        (void)I2C2->SR1;                     /* reading SR1 then SR2 clears ADDR */
        (void)I2C2->SR2;
        return 0U;
    }

    /* data phase ends with BTF (transferred) or AF (NACK) */
    (void)i2c_hw_wait_any(I2C_SR1_BTF, I2C_SR1_AF);

    if ((I2C2->SR1 & I2C_SR1_AF) != 0U)
    {
        __HAL_I2C_CLEAR_FLAG(&g_i2c_handle, I2C_SR1_AF);
        return 1U;
    }

    return 0U;
}

void i2c_ack(void)
{
    I2C2->CR1 |= I2C_CR1_ACK;
}

void i2c_nack(void)
{
    I2C2->CR1 &= ~I2C_CR1_ACK;
}

uint8_t i2c_read_byte(uint8_t ack)
{
    /* The ACK bit present while a byte is received is the ACK/NACK sent after
     * it, so program it before waiting for the data. */
    if (ack != 0U)
    {
        I2C2->CR1 |= I2C_CR1_ACK;
    }
    else
    {
        I2C2->CR1 &= ~I2C_CR1_ACK;
    }

    (void)i2c_hw_wait_any(I2C_SR1_RXNE, 0U);
    return (uint8_t)I2C2->DR;
}

#else  /* software bit-bang backend */

#include "delay.h"

#define I2C_DELAY_US      2U
#define I2C_ACK_TIMEOUT   250U

#define I2C_SCL(x)  do { (x) ? HAL_GPIO_WritePin(I2C_SCL_GPIO_PORT, I2C_SCL_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(I2C_SCL_GPIO_PORT, I2C_SCL_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define I2C_SDA(x)  do { (x) ? HAL_GPIO_WritePin(I2C_SDA_GPIO_PORT, I2C_SDA_GPIO_PIN, GPIO_PIN_SET) \
                             : HAL_GPIO_WritePin(I2C_SDA_GPIO_PORT, I2C_SDA_GPIO_PIN, GPIO_PIN_RESET); } while (0)

#define I2C_READ_SDA  HAL_GPIO_ReadPin(I2C_SDA_GPIO_PORT, I2C_SDA_GPIO_PIN)

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
    delay_us(I2C_DELAY_US);
    I2C_SDA(0);
    delay_us(I2C_DELAY_US);
    I2C_SCL(0);
    delay_us(I2C_DELAY_US);
}

void i2c_stop(void)
{
    I2C_SDA(0);
    delay_us(I2C_DELAY_US);
    I2C_SCL(1);
    delay_us(I2C_DELAY_US);
    I2C_SDA(1);
    delay_us(I2C_DELAY_US);
}

uint8_t i2c_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    I2C_SDA(1);
    delay_us(I2C_DELAY_US);
    I2C_SCL(1);
    delay_us(I2C_DELAY_US);

    while (I2C_READ_SDA)
    {
        waittime++;

        if (waittime > I2C_ACK_TIMEOUT)
        {
            i2c_stop();
            rack = 1;
            break;
        }

        delay_us(I2C_DELAY_US);
    }

    I2C_SCL(0);
    delay_us(I2C_DELAY_US);

    return rack;
}

void i2c_ack(void)
{
    I2C_SDA(0);
    delay_us(I2C_DELAY_US);
    I2C_SCL(1);
    delay_us(I2C_DELAY_US);
    I2C_SCL(0);
    delay_us(I2C_DELAY_US);
    I2C_SDA(1);
    delay_us(I2C_DELAY_US);
}

void i2c_nack(void)
{
    I2C_SDA(1);
    delay_us(I2C_DELAY_US);
    I2C_SCL(1);
    delay_us(I2C_DELAY_US);
    I2C_SCL(0);
    delay_us(I2C_DELAY_US);
}

void i2c_send_byte(uint8_t data)
{
    uint8_t t;

    for (t = 0; t < 8U; t++)
    {
        I2C_SDA((data & 0x80U) >> 7);
        delay_us(I2C_DELAY_US);
        I2C_SCL(1);
        delay_us(I2C_DELAY_US);
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
        delay_us(I2C_DELAY_US);

        if (I2C_READ_SDA != 0U)
        {
            receive++;
        }

        I2C_SCL(0);
        delay_us(I2C_DELAY_US);
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

#endif /* BSP_I2C_USE_HARDWARE */
