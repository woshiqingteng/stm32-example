/**
 * @file    main.c
 * @brief   53_iap_app: application image for the IAP bootloader, linked at
 *          0x08010000 (see LD stm32f429ig_iap_app.ld).
 */

#include <stdio.h>
#include "bsp.h"

#define LOOP_DELAY_MS 500U

int main(void)
{
    bsp_init();
    sys_set_vector_table(IAP_APP_ADDR);

    printf("53_iap_app running @ 0x%08X\r\n", (unsigned)IAP_APP_ADDR);

    for (;;)
    {
        led_toggle(LED0);
        printf("iap app alive\r\n");
        delay_ms(LOOP_DELAY_MS);
    }
}
