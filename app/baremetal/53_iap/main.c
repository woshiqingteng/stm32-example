/**
 * @file    main.c
 * @brief   53_iap: serial IAP bootloader. Receives a firmware image over
 *          USART1 and programs it at IAP_APP_ADDR, then jumps to it.
 */

#include <stdio.h>
#include "bsp.h"

int main(void)
{
    bsp_init();

    printf("53_iap bootloader ready, app @ 0x%08X\r\n", (unsigned)IAP_APP_ADDR);
    printf("send firmware frame: 5A A5 <len_lo> <len_hi> <data> <sum8>\r\n");

    for (;;)
    {
        led_toggle(LED0);

        if (iap_receive_usart() == IAP_OK)
        {
            printf("IAP: image received, jumping to app\r\n");
            iap_jump(IAP_APP_ADDR);
            printf("IAP: jump failed\r\n");
        }

        delay_ms(100);
    }
}
