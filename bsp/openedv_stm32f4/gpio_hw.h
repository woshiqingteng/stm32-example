/**
 * @file    gpio_hw.h
 * @brief   Generic GPIO hardware attribute (port + clock + electrical config).
 *
 * Shared by board drivers (ADC, USART, ...) that embed a gpio_hw_t to describe
 * how one GPIO port is configured. The pin mask is passed to gpio_hw_setup()
 * per use so a single descriptor can serve several pin groups.
 */

#ifndef BSP_GPIO_HW_H
#define BSP_GPIO_HW_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/** @brief GPIO hardware attributes for one port. */
typedef struct
{
    GPIO_TypeDef *port;      /*!< GPIOA / GPIOB / ... */
    uint32_t      rcc_en;    /*!< RCC AHB1ENR clock-enable bit */
    uint32_t      mode;      /*!< GPIO_MODE_* */
    uint32_t      pull;      /*!< GPIO_PULL* */
    uint32_t      speed;     /*!< GPIO_SPEED_FREQ_* */
    uint32_t      alternate; /*!< GPIO_AFx_* (0 for non-AF modes) */
} gpio_hw_t;

/**
 * @brief  Enable the port clock and configure @p pins with the descriptor.
 * @param  pins GPIO_PIN_x mask (0 is a no-op).
 */
static inline void gpio_hw_setup(const gpio_hw_t *hw, uint16_t pins)
{
    GPIO_InitTypeDef gpio = {0};

    if (pins == 0U)
    {
        return;
    }
    SET_BIT(RCC->AHB1ENR, hw->rcc_en);

    gpio.Pin       = pins;
    gpio.Mode      = hw->mode;
    gpio.Pull      = hw->pull;
    gpio.Speed     = hw->speed;
    gpio.Alternate = hw->alternate;
    HAL_GPIO_Init(hw->port, &gpio);
}

#endif /* BSP_GPIO_HW_H */
