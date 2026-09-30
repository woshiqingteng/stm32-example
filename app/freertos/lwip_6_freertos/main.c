/**
 * @file    main.c
 * @brief   lwip_6_freertos: lwIP bring-up (ALIENTEK experiment 6, "Ping Test").
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#include "lwip_comm.h"
#include "lwip_demo_ui.h"

#include "eth_phy.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024

static void demo_task(void *arg)
{
    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed(eth_phy_speed() == ETH_PHY_SPEED_100M ? "Ethernet Speed:100M"
                                                             : "Ethernet Speed:10M");

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    sdram_init();
    lcd_init();
    lcd_display_dir(LCD_DIR_PORTRAIT);
    lcd_clear(WHITE);

    if (lwip_comm_init() != 0)
    {
        lwip_demo_ui_retry();
        for (;;)
        {
        }
    }
    lwip_demo_ui_start("lwIP Ping Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
