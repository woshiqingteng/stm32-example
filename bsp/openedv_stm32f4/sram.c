/**
 * @file    sram.c
 * @brief   External SRAM driver over FMC (bank1, NE3), ported from the vendor
 *          SRAM example. MSP content (clocks + GPIO) is inlined into sram_init().
 */

#include "sram.h"

SRAM_HandleTypeDef g_sram_handler;

void sram_init(void)
{
    GPIO_InitTypeDef       gpio_init = {0};
    FMC_NORSRAM_TimingTypeDef timing  = {0};

    /* ---- MSP begin: FMC + data/address GPIO ---- */
    __HAL_RCC_FMC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF12_FMC;

    gpio_init.Pin = SRAM_CS_GPIO_PIN;
    HAL_GPIO_Init(SRAM_CS_GPIO_PORT, &gpio_init);
    gpio_init.Pin = SRAM_WR_GPIO_PIN;
    HAL_GPIO_Init(SRAM_WR_GPIO_PORT, &gpio_init);
    gpio_init.Pin = SRAM_RD_GPIO_PIN;
    HAL_GPIO_Init(SRAM_RD_GPIO_PORT, &gpio_init);

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
                    GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |
                    GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio_init);

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
                    GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                    GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio_init);

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                    GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 |
                    GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &gpio_init);

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                    GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOG, &gpio_init);
    /* ---- MSP end ---- */

    g_sram_handler.Instance = FMC_NORSRAM_DEVICE;
    g_sram_handler.Extended = FMC_NORSRAM_EXTENDED_DEVICE;

    g_sram_handler.Init.NSBank             = (SRAM_FMC_NEX == 1) ? FMC_NORSRAM_BANK1 :
                                             (SRAM_FMC_NEX == 2) ? FMC_NORSRAM_BANK2 :
                                             (SRAM_FMC_NEX == 3) ? FMC_NORSRAM_BANK3 :
                                                                    FMC_NORSRAM_BANK4;
    g_sram_handler.Init.DataAddressMux     = FMC_DATA_ADDRESS_MUX_DISABLE;
    g_sram_handler.Init.MemoryType         = FMC_MEMORY_TYPE_SRAM;
    g_sram_handler.Init.MemoryDataWidth    = FMC_NORSRAM_MEM_BUS_WIDTH_16;
    g_sram_handler.Init.BurstAccessMode    = FMC_BURST_ACCESS_MODE_DISABLE;
    g_sram_handler.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
    g_sram_handler.Init.WaitSignalActive   = FMC_WAIT_TIMING_BEFORE_WS;
    g_sram_handler.Init.WriteOperation     = FMC_WRITE_OPERATION_ENABLE;
    g_sram_handler.Init.WaitSignal         = FMC_WAIT_SIGNAL_DISABLE;
    g_sram_handler.Init.ExtendedMode       = FMC_EXTENDED_MODE_DISABLE;
    g_sram_handler.Init.AsynchronousWait   = FMC_ASYNCHRONOUS_WAIT_DISABLE;
    g_sram_handler.Init.WriteBurst         = FMC_WRITE_BURST_DISABLE;

    timing.AddressSetupTime      = 0x02;
    timing.AddressHoldTime       = 0x00;
    timing.DataSetupTime         = 0x08;
    timing.BusTurnAroundDuration = 0x00;
    timing.AccessMode            = FMC_ACCESS_MODE_A;

    (void)HAL_SRAM_Init(&g_sram_handler, &timing, &timing);
}

void sram_write(uint8_t *pbuf, uint32_t addr, uint32_t datalen)
{
    while (datalen-- != 0U)
    {
        *(volatile uint8_t *)(SRAM_BASE_ADDR + addr) = *pbuf;
        addr++;
        pbuf++;
    }
}

void sram_read(uint8_t *pbuf, uint32_t addr, uint32_t datalen)
{
    while (datalen-- != 0U)
    {
        *pbuf++ = *(volatile uint8_t *)(SRAM_BASE_ADDR + addr);
        addr++;
    }
}

void sram_test_write(uint32_t addr, uint8_t data)
{
    sram_write(&data, addr, 1U);
}

uint8_t sram_test_read(uint32_t addr)
{
    uint8_t data;

    sram_read(&data, addr, 1U);
    return data;
}

uint32_t sram_test(uint32_t addr, uint32_t len)
{
    uint32_t i;
    uint32_t errors = 0U;

    for (i = 0U; i < len; i++)
    {
        sram_test_write(addr + i, (uint8_t)(i * 7U + 3U));
    }

    for (i = 0U; i < len; i++)
    {
        if (sram_test_read(addr + i) != (uint8_t)(i * 7U + 3U))
        {
            errors++;
        }
    }

    return errors;
}
