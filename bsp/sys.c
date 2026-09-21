/**
 * @file    sys.c
 * @brief   System clock configuration (HSE 25MHz -> PLL).
 *
 * Example: plln=336, pllm=25, pllp=2, pllq=7 -> SYSCLK 168MHz, PLL48CK 48MHz.
 */

#include "stm32f4xx_hal.h"
#include "sys.h"

/* Upper bound on the HSI-ready poll so a dead oscillator cannot hang boot. */
#define SYS_HSI_READY_TIMEOUT 0xFFFFU

uint32_t sys_clk_get_hz(void)
{
    return HAL_RCC_GetHCLKFreq();
}

HAL_StatusTypeDef sys_clk_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq)
{
    RCC_OscInitTypeDef rcc_osc_init = {0};
    RCC_ClkInitTypeDef rcc_clk_init = {0};
    HAL_StatusTypeDef  status;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    rcc_osc_init.HSEState       = RCC_HSE_ON;
    rcc_osc_init.PLL.PLLState   = RCC_PLL_ON;
    rcc_osc_init.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    rcc_osc_init.PLL.PLLM       = pllm;
    rcc_osc_init.PLL.PLLN       = plln;
    rcc_osc_init.PLL.PLLP       = pllp;
    rcc_osc_init.PLL.PLLQ       = pllq;
    status = HAL_RCC_OscConfig(&rcc_osc_init);
    if (status != HAL_OK)
    {
        return status;
    }

    rcc_clk_init.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    rcc_clk_init.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    rcc_clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV4;
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV2;
    status = HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_5);
    if (status != HAL_OK)
    {
        return status;
    }

    SCB->VTOR = FLASH_BASE;

    return HAL_OK;
}

HAL_StatusTypeDef sys_clk_reconfig(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq)
{
    RCC_ClkInitTypeDef rcc_clk_init = {0};
    uint32_t           retry        = SYS_HSI_READY_TIMEOUT;
    HAL_StatusTypeDef  status;

    __HAL_RCC_HSI_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
    {
        if (retry-- == 0U)
        {
            return HAL_TIMEOUT;
        }
    }

    rcc_clk_init.ClockType      = RCC_CLOCKTYPE_SYSCLK;
    rcc_clk_init.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    rcc_clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV1;
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;
    status = HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_0);
    if (status != HAL_OK)
    {
        return status;
    }

    return sys_clk_init(plln, pllm, pllp, pllq);
}
