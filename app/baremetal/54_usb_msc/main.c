/**
 * @file    main.c
 * @brief   54_usb_msc: USB device Mass Storage backed by the SD card. The card
 *          capacity is shown on the RGB panel and read/write activity is
 *          reported while the host accesses the disk.
 */

#include <stdio.h>

#include "bsp.h"
#include "sdio.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

#define TEXT_X          30U
#define TEXT_WIDTH      300U
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

    sdram_init();
    lcd_init();

    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "USB Card Reader TEST", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    printf("54_usb_msc ready\r\n");

    if (sdio_init() != 0U)
    {
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, "SD Card Error!", RED);
        printf("SD init failed\r\n");
    }
    else
    {
        sdio_get_card_info(&info);
        (void)sprintf(line, "SD Card Size: %lu MB", (unsigned long)info.total_size_mb);
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16, line, BLUE);
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
                lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB Connected   ", BLUE);
            }
            else
            {
                lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB DisConnected", RED);
            }
        }

        if (storage_status != g_usb_storage_state)
        {
            storage_status = g_usb_storage_state;

            if ((storage_status & USB_STORAGE_WRITING) != 0U)
            {
                lcd_show_string(TEXT_X, 150U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB Writing...", RED);
            }
            else if ((storage_status & USB_STORAGE_READING) != 0U)
            {
                lcd_show_string(TEXT_X, 150U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB Reading...", RED);
            }
            else
            {
                lcd_fill(TEXT_X, 150U, TEXT_X + TEXT_WIDTH, 166U, WHITE);
            }

            if ((storage_status & USB_STORAGE_WRITE_ERR) != 0U)
            {
                lcd_show_string(TEXT_X, 170U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB Write Err", RED);
            }

            if ((storage_status & USB_STORAGE_READ_ERR) != 0U)
            {
                lcd_show_string(TEXT_X, 190U, TEXT_WIDTH, 16U, LCD_FONT_SIZE_16,
                                "USB Read  Err", RED);
            }

            g_usb_storage_state = 0U;
        }

        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
