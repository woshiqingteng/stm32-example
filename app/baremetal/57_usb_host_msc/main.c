/**
 * @file    main.c
 * @brief   57_usb_host_msc: USB host Mass Storage. A USB flash drive is
 *          enumerated and its root directory is listed through FatFs; the
 *          logical drive "2:" is mapped to the USB disk by the FatFs port.
 */

#include <stdio.h>

#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "usbh_core.h"
#include "usbh_handle.h"
#include "usbh_msc.h"
#define USB_DRIVE       "2:"
#define BLINK_PERIOD_MS 500U
#define LOOP_DELAY_MS   10U

static void usbh_list_root(void)
{
    DIR     dir;
    FILINFO fno;
    FRESULT res;

    res = f_opendir(&dir, USB_DRIVE "/");

    if (res != FR_OK)
    {
        printf("USB disk opendir failed (%d)\r\n", (int)res);
        return;
    }

    printf("root of %s:\r\n", USB_DRIVE);

    for (;;)
    {
        res = f_readdir(&dir, &fno);

        if ((res != FR_OK) || (fno.fname[0] == 0))
        {
            break;
        }

        printf("  %s\r\n", fno.fname);
    }

    (void)f_closedir(&dir);
}

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
    uint32_t total = 0U;
    uint32_t free_kb = 0U;

    (void)phost;

    switch (id)
    {
        case HOST_USER_DISCONNECTION:
            (void)f_mount(0, USB_DRIVE, 1);
            printf("USB DisConnected\r\n");
            printf("USB disk removed\r\n");
            break;

        case HOST_USER_CLASS_ACTIVE:
            printf("USB Connected\r\n");

            if (f_mount(fs[2], USB_DRIVE, 1) != FR_OK)
            {
                printf("USB disk mount failed\r\n");
            }
            else
            {
                printf("FATFS OK\r\n");

                if (exfuns_get_free((uint8_t *)USB_DRIVE, &total, &free_kb) == 0U)
                {
                    printf("USB %lu MB free %lu MB\r\n",
                           (unsigned long)(total >> 10), (unsigned long)(free_kb >> 10));
                }

                usbh_list_root();
            }
            break;

        case HOST_USER_CONNECTION:
            printf("Device Attached\r\n");
            break;

        default:
            break;
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

    printf(APP_BANNER "\r\n");

    (void)exfuns_init();

    (void)USBH_Init(&g_hUSBHost, USBH_UserProcess, HOST_FS);
    (void)USBH_RegisterClass(&g_hUSBHost, USBH_MSC_CLASS);
    (void)USBH_Start(&g_hUSBHost);

    for (;;)
    {
        (void)USBH_Process(&g_hUSBHost);

        if (++blink >= (BLINK_PERIOD_MS / LOOP_DELAY_MS))
        {
            blink = 0U;
            led_toggle(LED0);
        }

        delay_ms(LOOP_DELAY_MS);
    }
}
