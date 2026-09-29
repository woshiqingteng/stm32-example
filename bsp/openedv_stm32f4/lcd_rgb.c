/**
 * @file    lcd_rgb.c
 * @brief   RGB panel driver: strap id detection and panel configuration.
 */

#include "stm32f4xx_hal.h"
#include "lcd_rgb.h"

/* Single supported panel: 4.3 inch, 800x480 (id 0x4384). */
static const lcd_rgb_cfg_t g_lcd_rgb_4384 =
{
    LCD_PANEL_ID_4384,
    LCD_PANEL_WIDTH_PX, LCD_PANEL_HEIGHT_PX,
    LCD_PANEL_HSW, LCD_PANEL_VSW,
    LCD_PANEL_HBP, LCD_PANEL_VBP,
    LCD_PANEL_HFP, LCD_PANEL_VFP,
    LCD_PANEL_PLLSAIN_RAW, LCD_PANEL_PLLSAIR_RAW, LCD_PANEL_PLLSAIDIVR_RAW,
    LTDC_PCPOLARITY_IPC
};

/* Panel id straps: PG6, PI2, PI7. */
static uint16_t lcd_rgb_id_decode(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint8_t idx;

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_init.Pin   = GPIO_PIN_6;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOG, &gpio_init);

    gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOI, &gpio_init);

    idx = (uint8_t)(HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_6) << LCD_PANEL_IDX_SHIFT_0);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_2) << LCD_PANEL_IDX_SHIFT_1);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_7) << LCD_PANEL_IDX_SHIFT_2);

    if (idx == LCD_PANEL_IDX_4384)
    {
        return LCD_PANEL_ID_4384;
    }

    return 0U;
}

const lcd_rgb_cfg_t *lcd_rgb_probe(void)
{
    if (lcd_rgb_id_decode() == LCD_PANEL_ID_4384)
    {
        return &g_lcd_rgb_4384;
    }

    return 0;
}
