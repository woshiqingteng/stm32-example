/**
 * @file    main.c
 * @brief   58_usb_host_hid: USB host HID. Reads mouse and keyboard reports
 *          from an attached device and prints/display them.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include "usbh_hid_keybd.h"
#include "usbh_hid_mouse.h"

#define BLINK_PERIOD_MS 500U

USBH_HandleTypeDef g_hUSBHost;

static bool g_hid_ready = false;

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
    (void)phost;

    switch (id)
    {
        case HOST_USER_DISCONNECTION:
            g_hid_ready = false;
            printf("USB DisConnected\r\n");
            printf("HID device removed\r\n");
            break;

        case HOST_USER_CLASS_ACTIVE:
            g_hid_ready = true;
            printf("USB Connected\r\n");

            if (USBH_HID_GetDeviceType(phost) == HID_KEYBOARD)
            {
                printf("USB Keyboard\r\n");
            }
            else if (USBH_HID_GetDeviceType(phost) == HID_MOUSE)
            {
                printf("USB Mouse\r\n");
            }
            else
            {
                printf("Unknown HID\r\n");
            }
            break;

        case HOST_USER_CONNECTION:
            printf("Device Attached\r\n");
            break;

        default:
            break;
    }
}

static void usbh_hid_demo(void)
{
    HID_TypeTypeDef type = USBH_HID_GetDeviceType(&g_hUSBHost);

    if (type == HID_KEYBOARD)
    {
        HID_KEYBD_Info_TypeDef *keys = USBH_HID_GetKeybdInfo(&g_hUSBHost);

        if (keys != NULL)
        {
            uint8_t c = USBH_HID_GetASCIICode(keys);

            if (c != 0U)
            {
                char line[32];

                (void)sprintf(line, "KEY '%c' (0x%02X)", (char)c, (unsigned)c);
                printf("%s\r\n", line);
            }
        }
    }
    else if (type == HID_MOUSE)
    {
        HID_MOUSE_Info_TypeDef *mouse = USBH_HID_GetMouseInfo(&g_hUSBHost);

        if (mouse != NULL)
        {
            char line[48];

            (void)sprintf(line, "X:%3u Y:%3u B:%u%u%u",
                          (unsigned)mouse->x, (unsigned)mouse->y,
                          (unsigned)mouse->buttons[0],
                          (unsigned)mouse->buttons[1],
                          (unsigned)mouse->buttons[2]);
            printf("%s\r\n", line);
        }
    }
    else
    {
        /* nothing to do for an unsupported device */
    }
}

int main(void)
{
    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U);
    usart_init(115200U);

    (void)pcf8574_init();

    printf("58_usb_host_hid ready\r\n");

    (void)USBH_Init(&g_hUSBHost, USBH_UserProcess, HOST_FS);
    (void)USBH_RegisterClass(&g_hUSBHost, USBH_HID_CLASS);
    (void)USBH_Start(&g_hUSBHost);

    for (;;)
    {
        (void)USBH_Process(&g_hUSBHost);

        if (g_hid_ready)
        {
            usbh_hid_demo();
        }

        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
