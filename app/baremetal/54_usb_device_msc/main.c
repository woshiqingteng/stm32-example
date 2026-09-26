/**
 * @file    main.c
 * @brief   54_usb_device_msc: USB device Mass Storage backed by the SD card. The card
 *          capacity and read/write activity are reported on USART1 while the
 *          host accesses the disk.
 */

#include <stdbool.h>
#include <stdio.h>

#include "bsp.h"
#include "nand_storage.h"
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
    char     line[48];

    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U);
    usart_init(115200U);

    printf("54_usb_device_msc ready\r\n");

    nand_storage_activate();

    if (sdio_init() != 0U)
    {
        printf("SD init failed\r\n");
    }
    else
    {
        sdio_get_card_info(&info);
        (void)sprintf(line, "SD Card Size: %lu MB", (unsigned long)info.total_size_mb);
        printf("%s\r\n", line);
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
            else
            {
                /* idle */
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
        else
        {
            /* no error */
        }

        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
