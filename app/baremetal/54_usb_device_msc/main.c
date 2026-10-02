/**
 * @file    main.c
 * @brief   54_usb_device_msc: USB device Mass Storage backed by the SD card. The card
 *          capacity and read/write activity are reported on USART1 while the
 *          host accesses the disk.
 */

#include <stdbool.h>
#include <stdio.h>

#include "bsp.h"
#include "sdio.h"
#include "nor.h"
#include "ftl.h"
#include "nand.h"
#include "usbd_core.h"
#include "usbd_handle.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

#define BLINK_PERIOD_MS 500U

int main(void)
{
    sd_card_info_t info;
    usbd_dev_state_t       usb_state = USBD_DEV_STATE_DISCONNECTED;
    usb_storage_activity_t storage_activity = USB_STORAGE_ACTIVITY_IDLE;
    bool                   first = true;

    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U); /* matches the sys_clk_reconfig above (168 MHz core, 48 MHz USB) */
    usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1) });

    printf(APP_BANNER "\r\n");


    if (sdio_init() != 0U)
    {
        printf("SD init failed\r\n");
    }
    else
    {
        sdio_get_card_info(&info);
        printf("SD Card Size: %lu MB\r\n", (unsigned long)info.total_size_mb);
    }

    /* Bring up every backing medium before USB starts so the MSC enumeration
     * (which must answer quickly) never has to scan/format them. */
    nor_init();
    (void)ftl_init();

    /* One-time: if the NAND FTL does not cover the full good-block capacity
     * (e.g. it was left half-initialised), rebuild it so the LUN reports the
     * maximum usable size.  Destructive (erases the NAND). */
    if ((uint32_t)nand_dev.valid_blocknum < ((uint32_t)nand_dev.good_blocknum * 90U / 100U))
    {
        printf("formatting NAND to full capacity...\r\n");
        (void)ftl_format();
    }

    (void)USBD_Init(&USBD_Device, &MSC_Desc, DEVICE_FS);
    (void)USBD_RegisterClass(&USBD_Device, USBD_MSC_CLASS);
    (void)USBD_MSC_RegisterStorage(&USBD_Device, &USBD_Storage_Interface_fops);
    (void)USBD_Start(&USBD_Device);

    for (;;)
    {
        if (first || (usb_state != g_device_state))
        {
            first = false;
            usb_state = g_device_state;
            printf(usb_state == USBD_DEV_STATE_CONNECTED ? "USB Connected\r\n"
                                                         : "USB DisConnected\r\n");
        }

        if (storage_activity != g_usb_storage_activity)
        {
            storage_activity = g_usb_storage_activity;

            if (storage_activity == USB_STORAGE_ACTIVITY_WRITING)
            {
                printf("USB Writing...\r\n");
            }
            else if (storage_activity == USB_STORAGE_ACTIVITY_READING)
            {
                printf("USB Reading...\r\n");
            }
        }

        if (g_usb_storage_error == USB_STORAGE_ERROR_WRITE)
        {
            printf("USB Write Err\r\n");
            g_usb_storage_error = USB_STORAGE_ERROR_NONE;
        }
        else if (g_usb_storage_error == USB_STORAGE_ERROR_READ)
        {
            printf("USB Read Err\r\n");
            g_usb_storage_error = USB_STORAGE_ERROR_NONE;
        }

        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
