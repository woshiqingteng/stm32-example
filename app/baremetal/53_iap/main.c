/**
 * @file    main.c
 * @brief   53_iap: serial IAP bootloader with manual control. WK_UP receives a
 *          firmware image over USART1 and programs it at IAP_APP_ADDR; KEY1
 *          runs the programmed application. No SRAM-app mode (no SRAM on the
 *          board).
 */

#include <stdio.h>
#include "bsp.h"

#define LOOP_DELAY_MS 100U

int main(void)
{
    bsp_init();

    printf("53_iap bootloader ready, app @ 0x%08X\r\n", (unsigned)IAP_APP_ADDR);
    printf("WKUP: receive+program   frame 5A A5 <len_lo> <len_hi> <data> <sum8>\r\n");
    printf("KEY1: run the application\r\n");

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY_WKUP)
        {
            printf("IAP: waiting for image...\r\n");

            if (iap_receive_usart() == IAP_OK)
            {
                printf("IAP: image programmed, press KEY1 to run\r\n");
            }
            else
            {
                printf("IAP: receive/program failed\r\n");
            }
        }
        else if (key == KEY1)
        {
            printf("IAP: jumping to app\r\n");
            iap_jump(IAP_APP_ADDR);
            printf("IAP: jump failed (no valid app)\r\n");
        }
        else
        {
            /* no key */
        }

        led_toggle(LED0);
        delay_ms(LOOP_DELAY_MS);
    }
}
