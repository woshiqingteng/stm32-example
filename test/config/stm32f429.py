"""
STM32F429 device file: register addresses / offsets only (CMSIS style).

This mirrors the parts of stm32f429xx.h that the tests poke/peek. It contains
NO test expectations.
"""

# --- GPIO --------------------------------------------------------------------
GPIOA_BASE = 0x40020000
GPIOB_BASE = 0x40020400
GPIOC_BASE = 0x40020800
GPIOD_BASE = 0x40020C00
GPIOE_BASE = 0x40021000
GPIOF_BASE = 0x40021400
GPIOG_BASE = 0x40021800
GPIOH_BASE = 0x40021C00

GPIO_MODER = 0x00
GPIO_OTYPER = 0x04
GPIO_OSPEEDR = 0x08
GPIO_PUPDR = 0x0C
GPIO_IDR = 0x10
GPIO_ODR = 0x14
GPIO_BSRR = 0x18
GPIO_AFRL = 0x20
GPIO_AFRH = 0x24

# --- RCC ---------------------------------------------------------------------
RCC_BASE = 0x40023800
RCC_CSR = RCC_BASE + 0x74

RCC_CSR_RMVF = 1 << 24
RCC_CSR_IWDGRSTF = 1 << 29
RCC_CSR_WWDGRSTF = 1 << 30

# --- TIM ---------------------------------------------------------------------
TIM1_BASE = 0x40010000
TIM2_BASE = 0x40000000
TIM3_BASE = 0x40000400
TIM5_BASE = 0x40000C00
TIM6_BASE = 0x40001000
TIM8_BASE = 0x40010400

TIM_CR1 = 0x00
TIM_DIER = 0x0C
TIM_SR = 0x10
TIM_CCMR1 = 0x18
TIM_CCMR2 = 0x1C
TIM_CCER = 0x20
TIM_CNT = 0x24
TIM_PSC = 0x28
TIM_ARR = 0x2C
TIM_RCR = 0x30
TIM_CCR1 = 0x34
TIM_CCR4 = 0x40
TIM_BDTR = 0x44

TIM_CR1_CEN = 1 << 0
TIM_CR1_OPM = 1 << 3
TIM_SR_CC1IF = 1 << 1
TIM_SR_UIF = 1 << 0

# --- USART -------------------------------------------------------------------
USART1_BASE = 0x40011000
