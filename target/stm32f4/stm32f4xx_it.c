/**
 * @file    stm32f4xx_it.c
 * @brief   Core exception handlers and weak aliases for peripheral IRQs.
 *
 * A peripheral handler defined in a bsp module overrides its weak alias here.
 */

#include "stm32f4xx_hal.h"

void Default_Handler(void)
{
    for (;;)
    {
    }
}

void NMI_Handler(void)
{
    for (;;)
    {
    }
}

void HardFault_Handler(void)
{
    for (;;)
    {
    }
}

void MemManage_Handler(void)
{
    for (;;)
    {
    }
}

void BusFault_Handler(void)
{
    for (;;)
    {
    }
}

void UsageFault_Handler(void)
{
    for (;;)
    {
    }
}

void DebugMon_Handler(void)
{
    for (;;)
    {
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

#define WEAK_IRQ(name) void name(void) __attribute__((weak, alias("Default_Handler")))

WEAK_IRQ(SVC_Handler);
WEAK_IRQ(PendSV_Handler);
WEAK_IRQ(WWDG_IRQHandler);
WEAK_IRQ(PVD_IRQHandler);
WEAK_IRQ(TAMP_STAMP_IRQHandler);
WEAK_IRQ(RTC_WKUP_IRQHandler);
WEAK_IRQ(FLASH_IRQHandler);
WEAK_IRQ(RCC_IRQHandler);
WEAK_IRQ(EXTI0_IRQHandler);
WEAK_IRQ(EXTI1_IRQHandler);
WEAK_IRQ(EXTI2_IRQHandler);
WEAK_IRQ(EXTI3_IRQHandler);
WEAK_IRQ(EXTI4_IRQHandler);
WEAK_IRQ(DMA1_Stream0_IRQHandler);
WEAK_IRQ(DMA1_Stream1_IRQHandler);
WEAK_IRQ(DMA1_Stream2_IRQHandler);
WEAK_IRQ(DMA1_Stream3_IRQHandler);
WEAK_IRQ(DMA1_Stream4_IRQHandler);
WEAK_IRQ(DMA1_Stream5_IRQHandler);
WEAK_IRQ(DMA1_Stream6_IRQHandler);
WEAK_IRQ(ADC_IRQHandler);
WEAK_IRQ(CAN1_TX_IRQHandler);
WEAK_IRQ(CAN1_RX0_IRQHandler);
WEAK_IRQ(CAN1_RX1_IRQHandler);
WEAK_IRQ(CAN1_SCE_IRQHandler);
WEAK_IRQ(EXTI9_5_IRQHandler);
WEAK_IRQ(TIM1_BRK_TIM9_IRQHandler);
WEAK_IRQ(TIM1_UP_TIM10_IRQHandler);
WEAK_IRQ(TIM1_TRG_COM_TIM11_IRQHandler);
WEAK_IRQ(TIM1_CC_IRQHandler);
WEAK_IRQ(TIM2_IRQHandler);
WEAK_IRQ(TIM3_IRQHandler);
WEAK_IRQ(TIM4_IRQHandler);
WEAK_IRQ(I2C1_EV_IRQHandler);
WEAK_IRQ(I2C1_ER_IRQHandler);
WEAK_IRQ(I2C2_EV_IRQHandler);
WEAK_IRQ(I2C2_ER_IRQHandler);
WEAK_IRQ(SPI1_IRQHandler);
WEAK_IRQ(SPI2_IRQHandler);
WEAK_IRQ(USART1_IRQHandler);
WEAK_IRQ(USART2_IRQHandler);
WEAK_IRQ(USART3_IRQHandler);
WEAK_IRQ(EXTI15_10_IRQHandler);
WEAK_IRQ(RTC_Alarm_IRQHandler);
WEAK_IRQ(OTG_FS_WKUP_IRQHandler);
WEAK_IRQ(TIM8_BRK_TIM12_IRQHandler);
WEAK_IRQ(TIM8_UP_TIM13_IRQHandler);
WEAK_IRQ(TIM8_TRG_COM_TIM14_IRQHandler);
WEAK_IRQ(TIM8_CC_IRQHandler);
WEAK_IRQ(DMA1_Stream7_IRQHandler);
WEAK_IRQ(FMC_IRQHandler);
WEAK_IRQ(SDIO_IRQHandler);
WEAK_IRQ(TIM5_IRQHandler);
WEAK_IRQ(SPI3_IRQHandler);
WEAK_IRQ(UART4_IRQHandler);
WEAK_IRQ(UART5_IRQHandler);
WEAK_IRQ(TIM6_DAC_IRQHandler);
WEAK_IRQ(TIM7_IRQHandler);
WEAK_IRQ(DMA2_Stream0_IRQHandler);
WEAK_IRQ(DMA2_Stream1_IRQHandler);
WEAK_IRQ(DMA2_Stream2_IRQHandler);
WEAK_IRQ(DMA2_Stream3_IRQHandler);
WEAK_IRQ(DMA2_Stream4_IRQHandler);
WEAK_IRQ(ETH_IRQHandler);
WEAK_IRQ(ETH_WKUP_IRQHandler);
WEAK_IRQ(CAN2_TX_IRQHandler);
WEAK_IRQ(CAN2_RX0_IRQHandler);
WEAK_IRQ(CAN2_RX1_IRQHandler);
WEAK_IRQ(CAN2_SCE_IRQHandler);
WEAK_IRQ(OTG_FS_IRQHandler);
WEAK_IRQ(DMA2_Stream5_IRQHandler);
WEAK_IRQ(DMA2_Stream6_IRQHandler);
WEAK_IRQ(DMA2_Stream7_IRQHandler);
WEAK_IRQ(USART6_IRQHandler);
WEAK_IRQ(I2C3_EV_IRQHandler);
WEAK_IRQ(I2C3_ER_IRQHandler);
WEAK_IRQ(OTG_HS_EP1_OUT_IRQHandler);
WEAK_IRQ(OTG_HS_EP1_IN_IRQHandler);
WEAK_IRQ(OTG_HS_WKUP_IRQHandler);
WEAK_IRQ(OTG_HS_IRQHandler);
WEAK_IRQ(DCMI_IRQHandler);
WEAK_IRQ(HASH_RNG_IRQHandler);
WEAK_IRQ(FPU_IRQHandler);
WEAK_IRQ(UART7_IRQHandler);
WEAK_IRQ(UART8_IRQHandler);
WEAK_IRQ(SPI4_IRQHandler);
WEAK_IRQ(SPI5_IRQHandler);
WEAK_IRQ(SPI6_IRQHandler);
WEAK_IRQ(SAI1_IRQHandler);
WEAK_IRQ(LTDC_IRQHandler);
WEAK_IRQ(LTDC_ER_IRQHandler);
WEAK_IRQ(DMA2D_IRQHandler);
