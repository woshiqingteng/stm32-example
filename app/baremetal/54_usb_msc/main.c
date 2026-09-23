/**
 * @file    main.c
 * @brief   54_usb_msc: USB device Mass Storage backed by the SD card. The card
 *          capacity and read/write activity are reported on USART1 while the
 *          host accesses the disk.
 */

#include <stdio.h>

#include "bsp.h"
#include "sdio.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

#define BLINK_PERIOD_MS 500U

USBD_HandleTypeDef USBD_Device;

int main(void)
{
    sd_card_info_t info;
    uint8_t  usb_status = 0xFFU;
    uint8_t  storage_status = 0xFFU;
    char     line[48];

    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U);
    usart_init(115200U);

    printf("54_usb_msc ready\r\n");

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
        if (usb_status != g_device_state)
        {
            usb_status = g_device_state;

            if (usb_status == 1U)
            {
                printf("USB Connected\r\n");
            }
            else
            {
                printf("USB DisConnected\r\n");
            }
        }

        if (storage_status != g_usb_storage_state)
        {
            storage_status = g_usb_storage_state;

            if ((storage_status & USB_STORAGE_WRITING) != 0U)
            {
                printf("USB Writing...\r\n");
            }
            else if ((storage_status & USB_STORAGE_READING) != 0U)
            {
                printf("USB Reading...\r\n");
            }
            else
            {
                /* idle */
            }

            if ((storage_status & USB_STORAGE_WRITE_ERR) != 0U)
            {
                printf("USB Write Err\r\n");
            }

            if ((storage_status & USB_STORAGE_READ_ERR) != 0U)
            {
                printf("USB Read Err\r\n");
            }

            g_usb_storage_state = 0U;
        }

        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
