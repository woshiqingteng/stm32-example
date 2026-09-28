/**
 * @file    main.c
 * @brief   55_usb_device_audio: USB device Audio (speaker) with a local microphone
 *          monitor loopback. KEY0/WK_UP raise the volume, KEY2 lowers it.
 */

#include <stdio.h>

#include "bsp.h"
#include "usbd_core.h"
#include "usbd_handle.h"
#include "usbd_desc.h"
#include "usbd_audio.h"
#include "usbd_audio_if.h"

#define BLINK_PERIOD_MS 200U

int main(void)
{
    usbd_dev_state_t usb_status = USBD_DEV_STATE_DISCONNECTED;
    uint8_t  volume = 70U;
    key_id_t key;

    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U); /* matches the sys_clk_reconfig above (168 MHz core, 48 MHz USB) */
    usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1) });

    printf(APP_BANNER "\r\n");

    (void)USBD_Init(&USBD_Device, &AUDIO_Desc, DEVICE_FS);
    (void)USBD_RegisterClass(&USBD_Device, USBD_AUDIO_CLASS);
    (void)USBD_AUDIO_RegisterInterface(&USBD_Device, &USBD_AUDIO_fops);
    (void)USBD_Start(&USBD_Device);

    for (;;)
    {
        key = key_scan(true);

        if (key == KEY0)
        {
            if (volume < 100U)
            {
                volume++;
            }
        }
        else if (key == KEY2)
        {
            if (volume > 0U)
            {
                volume--;
            }
        }
        else if (key == KEY_WKUP)
        {
            volume = 70U;
        }
        if (key != KEY_NONE)
        {
            (void)BSP_AUDIO_OUT_SetVolume(volume);
            printf("volume %u\r\n", (unsigned)volume);
        }

        if (usb_status != g_device_state)
        {
            usb_status = g_device_state;

            if (usb_status == USBD_DEV_STATE_CONNECTED)
            {
                printf("USB Connected\r\n");
                led_on(LED1);
            }
            else
            {
                printf("USB DisConnected\r\n");
                led_off(LED1);
            }
        }

        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
