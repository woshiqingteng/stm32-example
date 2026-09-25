/**
 * @file    usbh_conf.c
 * @brief   STM32F4 USB OTG FS host low level driver (HCD) plus the low level
 *          hooks expected by the ST USB host library. Ported from the vendor
 *          usbh_conf.c. The OTG FS power switch on the board is driven through
 *          the PCF8574 IO expander.
 */

#include "usbh_conf.h"
#include "usbh_core.h"
#include "usbh_msc.h"
#include "usbh_hid.h"
#include "pcf8574.h"
#include "delay.h"

/* USB OTG FS host instance. */
HCD_HandleTypeDef g_hhcd_USB_OTG_FS;

USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status);

/* Single static class allocation (one host class per application). */
typedef union
{
    MSC_HandleTypeDef msc;
    HID_HandleTypeDef hid;
} usbh_class_mem_t;

static usbh_class_mem_t g_usbh_class_mem;

void *usbh_static_malloc(uint32_t size)
{
    (void)size;
    return (void *)&g_usbh_class_mem;
}

void usbh_static_free(void *p)
{
    (void)p;
}

void OTG_FS_IRQHandler(void)
{
    HAL_HCD_IRQHandler(&g_hhcd_USB_OTG_FS);
}

/* HCD to USB host library callbacks. */

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
    USBH_LL_IncTimer(hhcd->pData);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
    USBH_LL_Connect(hhcd->pData);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
    USBH_LL_Disconnect(hhcd->pData);
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
    USBH_LL_PortEnabled(hhcd->pData);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
    USBH_LL_PortDisabled(hhcd->pData);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum,
                                         HCD_URBStateTypeDef urb_state)
{
    (void)hhcd;
    (void)chnum;
    (void)urb_state;
}

/* Low level driver entry points used by the USB host library. */

USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
    if (phost->id == HOST_FS)
    {
        g_hhcd_USB_OTG_FS.Instance                 = USB_OTG_FS;
        g_hhcd_USB_OTG_FS.Init.Host_channels       = 11U;
        g_hhcd_USB_OTG_FS.Init.speed               = HCD_SPEED_FULL;
        g_hhcd_USB_OTG_FS.Init.dma_enable          = DISABLE;
        g_hhcd_USB_OTG_FS.Init.phy_itface          = HCD_PHY_EMBEDDED;
        g_hhcd_USB_OTG_FS.Init.Sof_enable          = DISABLE;
        g_hhcd_USB_OTG_FS.Init.vbus_sensing_enable = 0U;
        g_hhcd_USB_OTG_FS.Init.lpm_enable          = 0U;
        g_hhcd_USB_OTG_FS.Init.low_power_enable    = 0U;

        g_hhcd_USB_OTG_FS.pData  = phost;
        phost->pData             = &g_hhcd_USB_OTG_FS;

        /* ---- MSP begin: OTG FS clock + PA11/PA12 + VBUS switch + NVIC ---- */
        {
            GPIO_InitTypeDef gpio_init = {0};

            __HAL_RCC_GPIOA_CLK_ENABLE();
            __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

            gpio_init.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
            gpio_init.Mode      = GPIO_MODE_AF_PP;
            gpio_init.Pull      = GPIO_NOPULL;
            gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
            gpio_init.Alternate = GPIO_AF10_OTG_FS;
            HAL_GPIO_Init(GPIOA, &gpio_init);

            /* Cycle the host VBUS switch so a freshly attached device is reset. */
            pcf8574_write_bit(PCF8574_USB_PWR_IO, 0U);
            delay_ms(500U);
            pcf8574_write_bit(PCF8574_USB_PWR_IO, 1U);

            HAL_NVIC_SetPriority(OTG_FS_IRQn, 1U, 0U);
            HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
        }
        /* ---- MSP end ---- */

        (void)HAL_HCD_Init(&g_hhcd_USB_OTG_FS);
        USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&g_hhcd_USB_OTG_FS));
    }

    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
    /* ---- MSP begin: release OTG FS clock + GPIO + NVIC ---- */
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    /* ---- MSP end ---- */

    return USBH_Get_USB_Status(HAL_HCD_DeInit(phost->pData));
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
    return USBH_Get_USB_Status(HAL_HCD_Start(phost->pData));
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
    return USBH_Get_USB_Status(HAL_HCD_Stop(phost->pData));
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
    USBH_SpeedTypeDef speed;

    switch (HAL_HCD_GetCurrentSpeed(phost->pData))
    {
        case USBH_SPEED_HIGH:
            speed = USBH_SPEED_HIGH;
            break;

        case USBH_SPEED_LOW:
            speed = USBH_SPEED_LOW;
            break;

        case USBH_SPEED_FULL:
        default:
            speed = USBH_SPEED_FULL;
            break;
    }

    return speed;
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
    return USBH_Get_USB_Status(HAL_HCD_ResetPort(phost->pData));
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    return HAL_HCD_HC_GetXferCount(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe_num,
                                    uint8_t epnum, uint8_t dev_address, uint8_t speed,
                                    uint8_t ep_type, uint16_t mps)
{
    return USBH_Get_USB_Status(HAL_HCD_HC_Init(phost->pData, pipe_num, epnum,
                                               dev_address, speed, ep_type, mps));
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    return USBH_Get_USB_Status(HAL_HCD_HC_Halt(phost->pData, pipe));
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe,
                                     uint8_t direction, uint8_t ep_type, uint8_t token,
                                     uint8_t *pbuff, uint16_t length, uint8_t do_ping)
{
    return USBH_Get_USB_Status(HAL_HCD_HC_SubmitRequest(phost->pData, pipe, direction,
                                                        ep_type, token, pbuff, length,
                                                        do_ping));
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
    (void)phost;
    (void)state;
    USBH_Delay(200U);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
    HCD_HandleTypeDef *pHandle = (HCD_HandleTypeDef *)phost->pData;

    if (pHandle->hc[pipe].ep_is_in)
    {
        pHandle->hc[pipe].toggle_in = toggle;
    }
    else
    {
        pHandle->hc[pipe].toggle_out = toggle;
    }

    return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
    HCD_HandleTypeDef *pHandle = (HCD_HandleTypeDef *)phost->pData;

    if (pHandle->hc[pipe].ep_is_in)
    {
        return pHandle->hc[pipe].toggle_in;
    }

    return pHandle->hc[pipe].toggle_out;
}

void USBH_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
    USBH_StatusTypeDef usb_status;

    switch (hal_status)
    {
        case HAL_OK:
            usb_status = USBH_OK;
            break;

        case HAL_BUSY:
            usb_status = USBH_BUSY;
            break;

        case HAL_ERROR:
        case HAL_TIMEOUT:
        default:
            usb_status = USBH_FAIL;
            break;
    }

    return usb_status;
}
