/**
 * @file    main.c
 * @brief   lwip_14_sntp: SNTP client using lwIP's sntp app (experiment 14).
 */

#include <stdint.h>
#include <stdio.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/sntp.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  512

#define SNTP_SERVER_NAME    "ntp1.aliyun.com"

/* Called by lwIP SNTP (via SNTP_SET_SYSTEM_TIME in lwipopts.h). */
volatile uint32_t g_sntp_time;
void lwip_sntp_set_time(uint32_t sec)
{
    g_sntp_time = sec;
}

static void demo_task(void *arg)
{
    uint32_t last = 0;

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, SNTP_SERVER_NAME);
    sntp_init();
    printf("sntp: polling %s\r\n", SNTP_SERVER_NAME);

    for (;;)
    {
        if (g_sntp_time != 0U && g_sntp_time != last)
        {
            last = g_sntp_time;
            printf("sntp: unix time %lu\r\n", (unsigned long)g_sntp_time);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    lwip_comm_init();

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
