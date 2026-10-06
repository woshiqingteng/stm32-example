/**
 * @file    cherryusb_glue.c
 * @brief   Board low-level glue for the CherryUSB DWC2 port: the STM32 HAL MSP
 *          callbacks that bring up the USB OTG FS clock, the PA11/PA12 pins and
 *          the OTG interrupt. The register-level controller init is done by
 *          CherryUSB (port/dwc2), this file only provides the MSP hooks it
 *          calls (HAL_PCD_MspInit / HAL_HCD_MspInit).
 */

#include "stm32f4xx_hal.h"

/* The USB ISR drives the CherryUSB class callbacks, which post to FreeRTOS
 * (message queues / semaphores) from interrupt context, so the NVIC priority
 * must be numerically >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (= 5). */
#define CHERRYUSB_IRQ_PRIORITY   5U
#define CHERRYUSB_IRQ_SUBPRIORITY 0U

/* Anchor referenced by the linker (-Wl,-u) so this object is always pulled out
 * of the archive; otherwise the ST HAL's weak empty HAL_PCD_MspInit wins and the
 * OTG clock/pins are never set up. */
void cherryusb_glue_anchor(void)
{
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance == USB_OTG_FS)
    {
        GPIO_InitTypeDef gpio_init = {0};

        __HAL_RCC_GPIOA_CLK_ENABLE();

        gpio_init.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_NOPULL;
        gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF10_OTG_FS;
        HAL_GPIO_Init(GPIOA, &gpio_init);

        __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

        HAL_NVIC_SetPriority(OTG_FS_IRQn, CHERRYUSB_IRQ_PRIORITY, CHERRYUSB_IRQ_SUBPRIORITY);
        HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
    }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance == USB_OTG_FS)
    {
        __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
        HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    }
}

/* Host role is not built for this board (OTG_FS is device-only under this
 * bring-up), but the shared DWC2 glue references these weakly. Provide safe
 * stubs so the object is self-contained. */
void HAL_HCD_MspInit(HCD_HandleTypeDef *hhcd)
{
    (void)hhcd;
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hhcd)
{
    (void)hhcd;
}
