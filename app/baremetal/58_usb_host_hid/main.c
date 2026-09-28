/**
 * @file    main.c
 * @brief   58_usb_host_hid: USB host HID. Mouse motion is accumulated and
 *          clamped to the panel; keyboard characters are collected into an
 *          edit buffer (backspace supported). A periodic re-arm recovers from
 *          a dead enumeration.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "usbh_core.h"
#include "usbh_handle.h"
#include "usbh_hid.h"
#include "usbh_hid_keybd.h"
#include "usbh_hid_mouse.h"

#define BLINK_PERIOD_MS     500U
#define LOOP_DELAY_MS       10U
#define MOUSE_MAX_X         799
#define MOUSE_MAX_Y         479
#define KBD_LINE_MAX_BYTE        64U
#define RECONNECT_TIMEOUT_MS 2000U

static bool    g_hid_ready = false;
static uint32_t g_lost_tick;

static int32_t g_mouse_x = 400;
static int32_t g_mouse_y = 240;
static char    g_kbd_line[KBD_LINE_MAX_BYTE];
static uint16_t g_kbd_len;

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
    (void)phost;

    switch (id)
    {
        case HOST_USER_DISCONNECTION:
            g_hid_ready = false;
            g_lost_tick = sys_get_tick();
            printf("USB DisConnected\r\n");
            break;

        case HOST_USER_CLASS_ACTIVE:
            g_hid_ready = true;
            printf("USB Connected\r\n");

            if (USBH_HID_GetDeviceType(phost) == HID_KEYBOARD)
            {
                g_kbd_len = 0U;
                g_kbd_line[0] = '\0';
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

            if (c == 0x08U)
            {
                if (g_kbd_len > 0U)
                {
                    g_kbd_len--;
                    g_kbd_line[g_kbd_len] = '\0';
                    printf("KBD: %s\r\n", g_kbd_line);
                }
            }
            else if ((c >= 0x20U) && (c <= 0x7EU) && (g_kbd_len < (KBD_LINE_MAX_BYTE - 1U)))
            {
                g_kbd_line[g_kbd_len++] = (char)c;
                g_kbd_line[g_kbd_len] = '\0';
                printf("KBD: %s\r\n", g_kbd_line);
            }
        }
    }
    else if (type == HID_MOUSE)
    {
        HID_MOUSE_Info_TypeDef *mouse = USBH_HID_GetMouseInfo(&g_hUSBHost);

        if (mouse != NULL)
        {
            g_mouse_x += mouse->x;
            g_mouse_y += mouse->y;

            if (g_mouse_x < 0) { g_mouse_x = 0; }
            if (g_mouse_x > MOUSE_MAX_X) { g_mouse_x = MOUSE_MAX_X; }
            if (g_mouse_y < 0) { g_mouse_y = 0; }
            if (g_mouse_y > MOUSE_MAX_Y) { g_mouse_y = MOUSE_MAX_Y; }

            printf("MOUSE X:%ld Y:%ld B:%u%u%u\r\n", (long)g_mouse_x, (long)g_mouse_y,
                   (unsigned)mouse->buttons[0], (unsigned)mouse->buttons[1],
                   (unsigned)mouse->buttons[2]);
        }
    }
}

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U); /* matches the sys_clk_reconfig above (168 MHz core, 48 MHz USB) */
    usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1) });

    (void)io_expand_init();

    printf(APP_BANNER "\r\n");

    (void)USBH_Init(&g_hUSBHost, USBH_UserProcess, HOST_FS);
    (void)USBH_RegisterClass(&g_hUSBHost, USBH_HID_CLASS);
    (void)USBH_Start(&g_hUSBHost);

    g_lost_tick = sys_get_tick();

    for (;;)
    {
        (void)USBH_Process(&g_hUSBHost);

        if (g_hid_ready)
        {
            usbh_hid_demo();
        }
        else if ((sys_get_tick() - g_lost_tick) >= RECONNECT_TIMEOUT_MS)
        {
            /* Re-arm the host after a failed/dead enumeration. */
            (void)USBH_Stop(&g_hUSBHost);
            (void)USBH_Start(&g_hUSBHost);
            g_lost_tick = sys_get_tick();
        }

        if (++blink >= (BLINK_PERIOD_MS / LOOP_DELAY_MS))
        {
            blink = 0U;
            led_toggle(LED0);
        }

        delay_ms(LOOP_DELAY_MS);
    }
}
