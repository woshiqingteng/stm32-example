/**
 * @file    main.c
 * @brief   lwip_13_ntp: NTP client (ALIENTEK experiment 13).
 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "rtc.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"

#include "lwip_comm.h"
#include "lwip_demo_ui.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024

static struct tm *lwip_utc8_gmtime(uint32_t utc_sec, struct tm *out)
{
    time_t t = (time_t)utc_sec + (time_t)(8 * 3600);
    struct tm *g = gmtime(&t);

    if (g != NULL)
    {
        *out = *g;
    }

    return out;
}

#define NTP_SERVER          "ntp1.aliyun.com"
#define NTP_PORT            123
#define NTP_MSG_LEN         48
#define NTP_TIMESTAMP_DELTA 2208988800U

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void demo_task(void *arg)
{
    uint8_t pkt[NTP_MSG_LEN];
    struct sockaddr_in server;
    struct hostent *he;
    int s;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    he = gethostbyname(NTP_SERVER);
    if (he == NULL)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(NTP_PORT);
    memcpy(&server.sin_addr, he->h_addr, he->h_length);

    memset(pkt, 0, sizeof(pkt));
    pkt[0] = 0x1BU;

    if (sendto(s, pkt, sizeof(pkt), 0, (struct sockaddr *)&server, sizeof(server)) == (int)sizeof(pkt))
    {
        if (recv(s, pkt, sizeof(pkt), 0) >= (int)sizeof(pkt))
        {
            time_t t = (time_t)(be32(&pkt[40]) - NTP_TIMESTAMP_DELTA);
            struct tm ti;
            char line[40];

            lwip_utc8_gmtime((uint32_t)t, &ti);

            rtc_set_date((uint8_t)(ti.tm_year + 1900 - 2000), (uint8_t)(ti.tm_mon + 1),
                         (uint8_t)ti.tm_mday, (uint8_t)(ti.tm_wday + 1));
            rtc_set_time((uint8_t)ti.tm_hour, (uint8_t)ti.tm_min, (uint8_t)ti.tm_sec, RTC_AM_24H);

            sprintf(line, "20%02d-%02d-%02d %02d:%02d:%02d",
                    ti.tm_year % 100, ti.tm_mon + 1, ti.tm_mday,
                    ti.tm_hour, ti.tm_min, ti.tm_sec);
            lwip_demo_ui_show(5, 150, 16, line, MAGENTA);
        }
    }
    closesocket(s);

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

    rtc_init();

    if (lwip_comm_init() != 0)
    {
        lwip_demo_ui_retry();
        for (;;)
        {
        }
    }
    lwip_demo_ui_start("lwIP NTP Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
