/**
 * @file    main.c
 * @brief   56_usb_cdc: USB device CDC virtual COM port. Bytes received on the
 *          virtual COM port are echoed to USART1 and shown on the RGB panel.
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U
#define BLINK_PERIOD_MS 500U

USBD_HandleTypeDef USBD_Device;

int main(void)
{
    uint8_t  usb_status = 0xFFU;
    uint16_t times = 0U;

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
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "USB Virtual COM TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);
    lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "USB Connecting...", RED);

    printf("56_usb_cdc ready\r\n");

    (void)USBD_Init(&USBD_Device, &VCP_Desc, DEVICE_FS);
    (void)USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
    (void)USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
    (void)USBD_Start(&USBD_Device);

    for (;;)
    {
        if (usb_status != g_device_state)
        {
            usb_status = g_device_state;

            if (usb_status == 1U)
            {
                lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB Connected   ", BLUE);
                led_on(LED1);
            }
            else
            {
                lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB DisConnected", RED);
                led_off(LED1);
            }
        }

        if ((g_usb_usart_rx_sta & 0x8000U) != 0U)
        {
            uint16_t len = g_usb_usart_rx_sta & 0x3FFFU;
            char     line[48];

            /* Echo the line to USART1 / LCD, then back to the host. */
            printf("usb rx %u bytes\r\n", (unsigned)len);
            (void)memcpy(line, g_usb_usart_rx_buffer, len);
            line[len] = '\0';
            lcd_fill(TEXT_X, 130U, TEXT_X + TEXT_WIDTH, 146U, WHITE);
            lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);

            cdc_vcp_data_tx(g_usb_usart_rx_buffer, len);
            g_usb_usart_rx_sta = 0U;
        }
        else
        {
            times++;

            if ((times % 5000U) == 0U)
            {
                usb_printf("\r\nSTM32 USB virtual COM test\r\n");
            }

            if ((times % 30U) == 0U)
            {
                led_toggle(LED0);
            }

            delay_ms(10U);
        }
    }
}
