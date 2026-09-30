/**
 * @file    sdram.c
 * @brief   On-board SDRAM (FMC SDRAM bank1): generic FMC bring-up and
 *          memory-mapped access. The concrete device parameters come from the
 *          chip header (sdram_w9825g6kh.h -> W9825G6KH-6, 8192x512x16, 32 MB).
 */

#include <stdio.h>

#include "sdram.h"
#include "sdram_w9825g6kh.h"
#include "delay.h"

#define SDRAM_CLK_ENABLE_DELAY_US 500U
#define SDRAM_COMMAND_TIMEOUT_COUNT     0x1000U

/* FMC commands issued during the initialisation sequence. */
typedef enum
{
    SDRAM_CMD_CLK_ENABLE  = FMC_SDRAM_CMD_CLK_ENABLE,
    SDRAM_CMD_PALL        = FMC_SDRAM_CMD_PALL,
    SDRAM_CMD_AUTOREFRESH = FMC_SDRAM_CMD_AUTOREFRESH_MODE,
    SDRAM_CMD_LOAD_MODE   = FMC_SDRAM_CMD_LOAD_MODE
} sdram_command_t;

/* Auto-refresh cycles: a single one for most commands, a burst of eight for
 * the auto-refresh command. */
typedef enum
{
    SDRAM_REFRESH_SINGLE    = 1,
    SDRAM_AUTOREFRESH_BURST = 8
} sdram_refresh_t;

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

static void sdram_send_command(sdram_command_t command, sdram_refresh_t refresh, uint16_t mode)
{
    FMC_SDRAM_CommandTypeDef cmd = {0};

    cmd.CommandMode            = (uint32_t)command;
    cmd.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    cmd.AutoRefreshNumber      = (uint32_t)refresh;
    cmd.ModeRegisterDefinition = mode;

    (void)HAL_SDRAM_SendCommand(&g_sdram_handle, &cmd, SDRAM_COMMAND_TIMEOUT_COUNT);
}

static void sdram_initialization_sequence(const sdram_cfg_t *cfg)
{
    /* JEDEC power-up: clock enable, precharge all, 8 auto-refreshes, load the
     * device mode register (burst length 1, sequential, CAS and write burst). */
    sdram_send_command(SDRAM_CMD_CLK_ENABLE, SDRAM_REFRESH_SINGLE, 0U);
    delay_us(SDRAM_CLK_ENABLE_DELAY_US);
    sdram_send_command(SDRAM_CMD_PALL, SDRAM_REFRESH_SINGLE, 0U);
    sdram_send_command(SDRAM_CMD_AUTOREFRESH, SDRAM_AUTOREFRESH_BURST, 0U);
    sdram_send_command(SDRAM_CMD_LOAD_MODE, SDRAM_REFRESH_SINGLE, cfg->mode_register);
}

/* Convert a nanosecond time to the FMC cycle count (ceil), clamped to the
 * 1..16 range that the FMC SDTR fields accept. */
static uint32_t ns_to_cycle(uint16_t ns, uint32_t sdclk_hz)
{
    uint32_t n = ((uint32_t)ns * (sdclk_hz / 1000000U) + 999U) / 1000U;

    if (n < 1U)
    {
        n = 1U;
    }

    if (n > 16U)
    {
        printf("sdram: timing over 16 cycles, clamped\r\n");
        n = 16U;
    }

    return n;
}

void sdram_init(void)
{
    const sdram_cfg_t *cfg = &g_sdram_w9825g6kh;
    FMC_SDRAM_TimingTypeDef timing = {0};
    uint32_t sdclk_hz;
    uint32_t refresh_count;

    sdram_gpio_init();

    g_sdram_handle.Instance                 = FMC_SDRAM_DEVICE;
    g_sdram_handle.Init.SDBank              = FMC_SDRAM_BANK1;
    g_sdram_handle.Init.ColumnBitsNumber    = cfg->col_bit;
    g_sdram_handle.Init.RowBitsNumber       = cfg->row_bit;
    g_sdram_handle.Init.MemoryDataWidth     = cfg->data_width;
    g_sdram_handle.Init.InternalBankNumber  = cfg->bank_num;
    g_sdram_handle.Init.CASLatency          = cfg->cas_latency;
    g_sdram_handle.Init.WriteProtection     = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    g_sdram_handle.Init.SDClockPeriod       = (cfg->sdclk_div == 3U) ? FMC_SDRAM_CLOCK_PERIOD_3
                                                                     : FMC_SDRAM_CLOCK_PERIOD_2;
    g_sdram_handle.Init.ReadBurst           = FMC_SDRAM_RBURST_ENABLE;
    g_sdram_handle.Init.ReadPipeDelay       = FMC_SDRAM_RPIPE_DELAY_1;

    /* SDCLK from the clock configured before this call (see the header). */
    sdclk_hz = HAL_RCC_GetHCLKFreq() / cfg->sdclk_div;

    timing.LoadToActiveDelay    = cfg->tmrd_cycle;
    timing.ExitSelfRefreshDelay = ns_to_cycle(cfg->txsr_ns, sdclk_hz);
    timing.SelfRefreshTime      = ns_to_cycle(cfg->tras_ns, sdclk_hz);
    timing.RowCycleDelay        = ns_to_cycle(cfg->trc_ns, sdclk_hz);
    timing.WriteRecoveryTime    = cfg->twr_cycle;
    timing.RPDelay              = ns_to_cycle(cfg->trp_ns, sdclk_hz);
    timing.RCDDelay             = ns_to_cycle(cfg->trcd_ns, sdclk_hz);

    /* The FMC requires TRC >= TRAS + TRP. */
    if (timing.RowCycleDelay < (timing.SelfRefreshTime + timing.RPDelay))
    {
        timing.RowCycleDelay = timing.SelfRefreshTime + timing.RPDelay;
    }

    (void)HAL_SDRAM_Init(&g_sdram_handle, &timing);
    sdram_initialization_sequence(cfg);

    /* COUNT = (tREF / row_num) * f_SDCLK - 20 (STM32 FMC refresh counter). */
    refresh_count = (((uint32_t)cfg->refresh_period_ms * (sdclk_hz / 1000U)) / cfg->row_num) - 20U;
    (void)HAL_SDRAM_ProgramRefreshRate(&g_sdram_handle, refresh_count);
}

void sdram_write_buffer(const uint8_t *src, uint32_t offset, uint32_t len)
{
    /* volatile: the SDRAM is memory-mapped I/O-like, so the accesses must not
     * be folded or reordered by the compiler. */
    volatile uint8_t *dst = (volatile uint8_t *)(SDRAM_BASE_ADDR + offset);

    while (len-- != 0U)
    {
        *dst++ = *src++;
    }
}

void sdram_read_buffer(uint8_t *dst, uint32_t offset, uint32_t len)
{
    /* volatile: keep the actual read accesses to the external SDRAM. */
    const volatile uint8_t *src = (const volatile uint8_t *)(SDRAM_BASE_ADDR + offset);

    while (len-- != 0U)
    {
        *dst++ = *src++;
    }
}
