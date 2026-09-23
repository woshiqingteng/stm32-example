/**
 * @file    main.c
 * @brief   56_usb_cdc: USB device CDC virtual COM port. Bytes received on the
 *          virtual COM port are echoed to USART1.
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#define BLINK_PERIOD_MS 500U
#define LOOP_DELAY_MS   10U
#define CDC_RX_READY_FLAG 0x8000U
#define CDC_RX_LEN_MASK   0x3FFFU

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
                printf("USB Connected\r\n");
                led_on(LED1);
            }
            else
            {
                printf("USB DisConnected\r\n");
                led_off(LED1);
            }
        }

        if ((g_usb_usart_rx_sta & CDC_RX_READY_FLAG) != 0U)
        {
            uint16_t len = g_usb_usart_rx_sta & CDC_RX_LEN_MASK;
            char     line[48];

            /* Echo the line to USART1, then back to the host. */
            (void)memcpy(line, g_usb_usart_rx_buffer, len);
            line[len] = '\0';
            printf("usb rx %u bytes: %s\r\n", (unsigned)len, line);

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

            delay_ms(LOOP_DELAY_MS);
        }
    }
}
