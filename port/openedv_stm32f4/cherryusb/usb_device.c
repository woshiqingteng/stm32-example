/**
 * @file    usb_device.c
 * @brief   Shared CherryUSB device helpers (see usb_device.h).
 */

#include <stdio.h>

#include "usb_device.h"
#include "usbd_core.h"

/* STM32 unique device id registers (used for the serial number string). */
#define USB_DEVICE_UID0  (*(const uint32_t *)0x1FFF7A10U)
#define USB_DEVICE_UID1  (*(const uint32_t *)0x1FFF7A14U)

static bool       s_connected;
static char       s_serial[25];
static const char s_langid[] = { (char)0x09, (char)0x04 };

const char *usb_device_string_desc(uint8_t speed, uint8_t index)
{
    (void)speed;

    switch (index)
    {
        case 0U:
            return s_langid;
        case 1U:
            return "STMicroelectronics";
        case 3U:
            (void)snprintf(s_serial, sizeof(s_serial), "%08lX%08lX",
                           (unsigned long)USB_DEVICE_UID0, (unsigned long)USB_DEVICE_UID1);
            return s_serial;
        default:
            return NULL;
    }
}

void usb_device_event(uint8_t event)
{
    switch (event)
    {
        case USBD_EVENT_RESET:
        case USBD_EVENT_DISCONNECTED:
            s_connected = false;
            break;
        case USBD_EVENT_CONFIGURED:
            s_connected = true;
            break;
        default:
            break;
    }
}

bool usb_device_connected(void)
{
    return s_connected;
}
