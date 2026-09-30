/**
 * @file    main.c
 * @brief   lwip_17_aliyun_mqtt: Aliyun IoT MQTT client (ALIENTEK experiment 17).
 *
 * Plain MQTT (port 1883). Password = lowercase hex of HMAC-SHA1(device_secret,
 * content). Temperature/humidity are random, as in the ALIENTEK demo.
 *
 * The credentials below are PLACEHOLDERS - replace them with your own.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "text.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/mqtt.h"
#include "lwip/netdb.h"

#include "lwip_comm.h"
#include "lwip_crypto.h"
#include "lwip_demo_ui.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024

/* ---- Placeholder credentials (replace before use) ---- */
#define PRODUCT_KEY         "YOUR_PRODUCT_KEY"
#define DEVICE_NAME         "YOUR_DEVICE_NAME"
#define DEVICE_SECRET       "YOUR_DEVICE_SECRET"

#define HOST_NAME           PRODUCT_KEY ".iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define HOST_PORT           1883
#define CONTENT             "clientId"DEVICE_NAME"deviceName"DEVICE_NAME"productKey"PRODUCT_KEY"timestamp789"
#define CLIENT_ID           DEVICE_NAME"|securemode=3,signmethod=hmacsha1,timestamp=789|"
#define USER_NAME           DEVICE_NAME"&"PRODUCT_KEY
#define DEVICE_PUBLISH      "/sys/"PRODUCT_KEY"/"DEVICE_NAME"/thing/event/property/post"
#define DEVICE_SUBSCRIBE    "/sys/"PRODUCT_KEY"/"DEVICE_NAME"/thing/service/property/set"

static mqtt_client_t *s_client;
static ip_addr_t s_broker;
static uint8_t s_publish_flag;

static void hex_str(const uint8_t *bin, int n, char *out)
{
    int i;

    for (i = 0; i < n; i++)
    {
        sprintf(out + i * 2, "%02x", bin[i]);
    }
    out[n * 2] = '\0';
}

static void pub_cb(void *arg, err_t err)
{
    (void)arg;
    if (err == ERR_OK)
    {
        xQueueSend(g_display_queue, "publish ok", 0);
    }
}

static void sub_cb(void *arg, err_t err)
{
    (void)arg;
    (void)err;
}

static void in_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    (void)arg;
    (void)data;
    (void)flags;
    (void)len;
}

static void in_pub_cb(void *arg, const char *topic, u32_t tot_len)
{
    (void)arg;
    (void)tot_len;
    xQueueSend(g_display_queue, (void *)(topic != NULL ? topic : ""), 0);
}

static void conn_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    (void)arg;

    if (status == MQTT_CONNECT_ACCEPTED)
    {
        lwip_demo_ui_state("State:Connection Successful", BLUE);
        mqtt_set_inpub_callback(client, in_pub_cb, in_data_cb, NULL);
        mqtt_subscribe(client, DEVICE_SUBSCRIBE, 1, sub_cb, NULL);
        s_publish_flag = 1U;
    }
    else
    {
        lwip_demo_ui_state("State:Disconnect", BLUE);
    }
}

static void demo_task(void *arg)
{
    struct hostent *he;
    struct mqtt_connect_client_info_t ci;
    uint8_t digest[LWIP_HMAC_SHA1_LEN];
    static char password[LWIP_HMAC_SHA1_LEN * 2 + 1];

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    he = gethostbyname(HOST_NAME);
    if (he == NULL)
    {
        lwip_demo_ui_retry();
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    memcpy(&s_broker, he->h_addr, he->h_length);

    (void)lwip_hmac_sha1((const uint8_t *)DEVICE_SECRET, strlen(DEVICE_SECRET),
                         (const uint8_t *)CONTENT, strlen(CONTENT), digest);
    hex_str(digest, LWIP_HMAC_SHA1_LEN, password);

    memset(&ci, 0, sizeof(ci));
    ci.client_id   = CLIENT_ID;
    ci.client_user = USER_NAME;
    ci.client_pass = password;
    ci.keep_alive  = 60;

    s_client = mqtt_client_new();
    mqtt_client_connect(s_client, &s_broker, HOST_PORT, conn_cb, NULL, &ci);

    for (;;)
    {
        if (s_publish_flag && mqtt_client_is_connected(s_client))
        {
            char payload[128];
            int temp = 30 + rand() % 10 + 1;
            int humi10 = 548 + (rand() % 100); /* 54.8 + rand */

            /* No %f in this build: format tenths by hand. */
            sprintf(payload,
                    "{\"params\":{\"CurrentTemperature\":+%d.0,\"RelativeHumidity\":%d.%d},"
                    "\"method\":\"thing.event.property.post\"}",
                    temp, humi10 / 10, humi10 % 10);

            mqtt_publish(s_client, DEVICE_PUBLISH, payload, strlen(payload), 1, 0, pub_cb, NULL);
            xQueueSend(g_display_queue, payload, 0);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
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
    g_lwip_font_ok = (fonts_init() == 0U) ? 1U : 0U;

    lwip_comm_init();
    lwip_demo_ui_start("lwIP MQTTAliyun");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
