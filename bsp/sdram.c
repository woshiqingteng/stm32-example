/**
 * @file    sdram.c
 * @brief   On-board SDRAM driver (FMC bank5, 32 MB, 16-bit bus).
 */

#include "sdram.h"
#include "delay.h"

/* SDRAM mode-register fields (IS42S16400 style device). */
#define SDRAM_MODE_BURST_LEN_1   0x0000U
#define SDRAM_MODE_BURST_SEQ     0x0000U
#define SDRAM_MODE_CAS_LATENCY_3 0x0030U
#define SDRAM_MODE_STANDARD      0x0000U
#define SDRAM_MODE_WRITEBURST_1  0x0200U

#define SDRAM_TARGET_BANK1       FMC_SDRAM_CMD_TARGET_BANK1

#define SDRAM_REFRESH_COUNT      730U  /* 64 ms / 8192 rows @ SDCLK 96 MHz */
#define SDRAM_CLK_ENABLE_DELAY_US 500U
#define SDRAM_AUTOREFRESH_NUM    8U
#define SDRAM_COMMAND_TIMEOUT    0x1000U

static SDRAM_HandleTypeDef g_sdram_handle;

static void sdram_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_FMC_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF12_FMC;

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
               GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
               GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
               GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
               GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &gpio);
}

static void sdram_send_command(uint8_t command, uint8_t refresh, uint16_t mode)
{
    FMC_SDRAM_CommandTypeDef cmd = {0};

    cmd.CommandMode            = command;
    cmd.CommandTarget          = SDRAM_TARGET_BANK1;
    cmd.AutoRefreshNumber      = refresh;
    cmd.ModeRegisterDefinition = mode;

    (void)HAL_SDRAM_SendCommand(&g_sdram_handle, &cmd, SDRAM_COMMAND_TIMEOUT);
}

static void sdram_initialization_sequence(void)
{
    uint16_t mode = SDRAM_MODE_BURST_LEN_1 | SDRAM_MODE_BURST_SEQ |
                    SDRAM_MODE_CAS_LATENCY_3 | SDRAM_MODE_STANDARD |
                    SDRAM_MODE_WRITEBURST_1;

    sdram_send_command(FMC_SDRAM_CMD_CLK_ENABLE, 1U, 0U);
    delay_us(SDRAM_CLK_ENABLE_DELAY_US);
    sdram_send_command(FMC_SDRAM_CMD_PALL, 1U, 0U);
    sdram_send_command(FMC_SDRAM_CMD_AUTOREFRESH_MODE, SDRAM_AUTOREFRESH_NUM, 0U);
    sdram_send_command(FMC_SDRAM_CMD_LOAD_MODE, 1U, mode);
}

void sdram_init(void)
{
    FMC_SDRAM_TimingTypeDef timing = {0};

    sdram_gpio_init();

    g_sdram_handle.Instance                 = FMC_SDRAM_DEVICE;
    g_sdram_handle.Init.SDBank              = FMC_SDRAM_BANK1;
    g_sdram_handle.Init.ColumnBitsNumber    = FMC_SDRAM_COLUMN_BITS_NUM_9;
    g_sdram_handle.Init.RowBitsNumber       = FMC_SDRAM_ROW_BITS_NUM_13;
    g_sdram_handle.Init.MemoryDataWidth     = FMC_SDRAM_MEM_BUS_WIDTH_16;
    g_sdram_handle.Init.InternalBankNumber  = FMC_SDRAM_INTERN_BANKS_NUM_4;
    g_sdram_handle.Init.CASLatency          = FMC_SDRAM_CAS_LATENCY_3;
    g_sdram_handle.Init.WriteProtection     = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    g_sdram_handle.Init.SDClockPeriod       = FMC_SDRAM_CLOCK_PERIOD_2;
    g_sdram_handle.Init.ReadBurst           = FMC_SDRAM_RBURST_ENABLE;
    g_sdram_handle.Init.ReadPipeDelay       = FMC_SDRAM_RPIPE_DELAY_1;

    timing.LoadToActiveDelay    = 2U;
    timing.ExitSelfRefreshDelay = 7U;
    timing.SelfRefreshTime      = 6U;
    timing.RowCycleDelay        = 6U;
    timing.WriteRecoveryTime    = 2U;
    timing.RPDelay              = 2U;
    timing.RCDDelay             = 2U;

    (void)HAL_SDRAM_Init(&g_sdram_handle, &timing);
    sdram_initialization_sequence();
    (void)HAL_SDRAM_ProgramRefreshRate(&g_sdram_handle, SDRAM_REFRESH_COUNT);
}

void sdram_write_buffer(const uint8_t *src, uint32_t offset, uint32_t len)
{
    volatile uint8_t *dst = (volatile uint8_t *)(SDRAM_BASE_ADDR + offset);

    while (len-- != 0U)
    {
        *dst++ = *src++;
    }
}

void sdram_read_buffer(uint8_t *dst, uint32_t offset, uint32_t len)
{
    const volatile uint8_t *src = (const volatile uint8_t *)(SDRAM_BASE_ADDR + offset);

    while (len-- != 0U)
    {
        *dst++ = *src++;
    }
}
