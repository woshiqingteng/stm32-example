/**
 * @file    main.c
 * @brief   55_usb_audio: USB device Audio (speaker) with a local microphone
 *          monitor loopback. KEY0/WK_UP raise the volume, KEY2 lowers it.
 */

#include <stdio.h>

#include "bsp.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_audio.h"
#include "usbd_audio_if.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U
#define BLINK_PERIOD_MS 200U

USBD_HandleTypeDef USBD_Device;

int main(void)
{
    uint8_t  usb_status = 0xFFU;
    uint8_t  volume = 70U;
    key_id_t key;

    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U);
    usart_init(115200U);

    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "USB Sound Card TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(TEXT_X, 90U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "KEY2:Vol-  KEY0:Vol+", RED);
    lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "VOLUME:", BLUE);
    lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "USB Connecting...", RED);

    printf("55_usb_audio ready\r\n");

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
        else
        {
            /* no key */
        }

        if (key != KEY_NONE)
        {
            (void)BSP_AUDIO_OUT_SetVolume(volume);
            lcd_show_num(TEXT_X + 56U, 110U, volume, 3U, LCD_FONT_SIZE_16, BLUE);
            printf("volume %u\r\n", (unsigned)volume);
        }

        if (usb_status != g_device_state)
        {
            usb_status = g_device_state;

            if (usb_status == 1U)
            {
                lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB Connected   ", BLUE);
                led_on(LED1);
            }
            else
            {
                lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB DisConnected", RED);
                led_off(LED1);
            }
        }

        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
