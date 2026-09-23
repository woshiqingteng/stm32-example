/**
 * @file    usbd_conf.c
 * @brief   STM32F4 USB OTG FS device low level driver (PCD) plus the low level
 *          hooks expected by the ST USB device library. Ported from the vendor
 *          usbd_conf.c.
 */

#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_msc.h"
#include "usbd_msc_bot.h"
#include "usbd_cdc.h"
#include "usbd_audio.h"

/* USB OTG FS peripheral instance. */
PCD_HandleTypeDef g_pcd_usb_otg_fs;

/* USB connection state: false = not connected, true = connected. */
volatile bool g_device_state = false;

static USBD_StatusTypeDef usbd_get_usb_status(HAL_StatusTypeDef hal_status);

/*
 * Single static class allocation. A device app registers exactly one class,
 * so the largest class handle is enough. The union guarantees the arena is
 * large enough and correctly aligned for whichever class is used.
 */
typedef union
{
    USBD_MSC_BOT_HandleTypeDef msc;
    USBD_CDC_HandleTypeDef     cdc;
    USBD_AUDIO_HandleTypeDef   audio;
} usbd_class_mem_t;

static usbd_class_mem_t g_usbd_class_mem;

void *usbd_static_malloc(uint32_t size)
{
    (void)size;
    return (void *)&g_usbd_class_mem;
}

void usbd_static_free(void *p)
{
    (void)p;
}

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&g_pcd_usb_otg_fs);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage((USBD_HandleTypeDef *)hpcd->pData, (uint8_t *)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage((USBD_HandleTypeDef *)hpcd->pData, epnum,
                         hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage((USBD_HandleTypeDef *)hpcd->pData, epnum,
                        hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;

    switch (hpcd->Init.speed)
    {
        case PCD_SPEED_HIGH:
            speed = USBD_SPEED_HIGH;
            break;

        case PCD_SPEED_FULL:
            speed = USBD_SPEED_FULL;
            break;

        default:
            speed = USBD_SPEED_FULL;
            break;
    }

    USBD_LL_SetSpeed((USBD_HandleTypeDef *)hpcd->pData, speed);
    USBD_LL_Reset((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
    g_device_state = false;
    USBD_LL_Suspend((USBD_HandleTypeDef *)hpcd->pData);
    __HAL_PCD_GATE_PHYCLOCK(hpcd);

    if (hpcd->Init.low_power_enable)
    {
        /* Required low-power entry: no HAL API for SCB deep-sleep bits. */
        SCB->SCR |= (uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
    }
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_Resume((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    g_device_state = true;
    USBD_LL_DevConnected((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    g_device_state = false;
    USBD_LL_DevDisconnected((USBD_HandleTypeDef *)hpcd->pData);
}

/* Low level driver entry points used by the USB device library. */

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    if (pdev->id == DEVICE_FS)
    {
        g_pcd_usb_otg_fs.pData = pdev;
        pdev->pData = &g_pcd_usb_otg_fs;

        g_pcd_usb_otg_fs.Instance                 = USB_OTG_FS;
        g_pcd_usb_otg_fs.Init.dev_endpoints       = 4U;
        g_pcd_usb_otg_fs.Init.speed               = PCD_SPEED_FULL;
        g_pcd_usb_otg_fs.Init.dma_enable          = DISABLE;
        g_pcd_usb_otg_fs.Init.phy_itface          = PCD_PHY_EMBEDDED;
        g_pcd_usb_otg_fs.Init.Sof_enable          = DISABLE;
        g_pcd_usb_otg_fs.Init.low_power_enable    = DISABLE;
        g_pcd_usb_otg_fs.Init.lpm_enable          = DISABLE;
        g_pcd_usb_otg_fs.Init.vbus_sensing_enable = DISABLE;
        g_pcd_usb_otg_fs.Init.use_dedicated_ep1   = DISABLE;

        /* ---- MSP begin: OTG FS clock + PA11/PA12 + NVIC ---- */
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

            HAL_NVIC_SetPriority(OTG_FS_IRQn, 0U, 0U);
            HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
        }
        /* ---- MSP end ---- */

        (void)HAL_PCD_Init(&g_pcd_usb_otg_fs);

        (void)HAL_PCDEx_SetRxFiFo(&g_pcd_usb_otg_fs, 0x80U);
        (void)HAL_PCDEx_SetTxFiFo(&g_pcd_usb_otg_fs, 0U, 0x40U);
        (void)HAL_PCDEx_SetTxFiFo(&g_pcd_usb_otg_fs, 1U, 0x80U);
    }

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    /* ---- MSP begin: release OTG FS clock + GPIO + NVIC ---- */
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    /* ---- MSP end ---- */

    return usbd_get_usb_status(HAL_PCD_DeInit(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    return usbd_get_usb_status(HAL_PCD_Start(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    return usbd_get_usb_status(HAL_PCD_Stop(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr,
                                  uint8_t ep_type, uint16_t ep_mps)
{
    return usbd_get_usb_status(HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type));
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return usbd_get_usb_status(HAL_PCD_EP_Close(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return usbd_get_usb_status(HAL_PCD_EP_Flush(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return usbd_get_usb_status(HAL_PCD_EP_SetStall(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return usbd_get_usb_status(HAL_PCD_EP_ClrStall(pdev->pData, ep_addr));
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)pdev->pData;

    if ((ep_addr & 0x80U) == 0x80U)
    {
        return hpcd->IN_ep[ep_addr & 0x7FU].is_stall;
    }

    return hpcd->OUT_ep[ep_addr & 0x7FU].is_stall;
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    g_device_state = true;
    return usbd_get_usb_status(HAL_PCD_SetAddress(pdev->pData, dev_addr));
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr,
                                    uint8_t *pbuf, uint32_t size)
{
    return usbd_get_usb_status(HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size));
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr,
                                          uint8_t *pbuf, uint32_t size)
{
    return usbd_get_usb_status(HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size));
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef *)pdev->pData, ep_addr);
}

void USBD_LL_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

static USBD_StatusTypeDef usbd_get_usb_status(HAL_StatusTypeDef hal_status)
{
    USBD_StatusTypeDef usb_status;

    switch (hal_status)
    {
        case HAL_OK:
            usb_status = USBD_OK;
            break;

        case HAL_BUSY:
            usb_status = USBD_BUSY;
            break;

        case HAL_ERROR:
        case HAL_TIMEOUT:
        default:
            usb_status = USBD_FAIL;
            break;
    }

    return usb_status;
}
