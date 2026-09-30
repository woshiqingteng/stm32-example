/**
 * @file    main.c
 * @brief   lwip_17_aliyun_mqtt: Aliyun IoT MQTT client (ALIENTEK experiment 17).
 *
 * Plain MQTT (port 1883). The login password is HMAC-SHA1(device_secret,
 * content) rendered as lowercase hex, computed with the port's mbedTLS helper.
 *
 * The credentials below are PLACEHOLDERS - replace them with your own.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/mqtt.h"
#include "lwip/netdb.h"

#include "lwip_comm.h"
#include "lwip_crypto.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  768

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
    printf("mqtt: publish err %d\r\n", (int)err);
}

static void sub_cb(void *arg, err_t err)
{
    (void)arg;
    printf("mqtt: subscribe err %d\r\n", (int)err);
}

static void in_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    (void)arg;
    (void)data;
    (void)flags;
    printf("mqtt: rx %u bytes\r\n", (unsigned)len);
}

static void in_pub_cb(void *arg, const char *topic, u32_t tot_len)
{
    (void)arg;
    printf("mqtt: rx topic %s len %u\r\n", topic, (unsigned)tot_len);
}

static void conn_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    (void)arg;
    printf("mqtt: connect status %d\r\n", (int)status);

    if (status == MQTT_CONNECT_ACCEPTED)
    {
        mqtt_set_inpub_callback(client, in_pub_cb, in_data_cb, NULL);
        mqtt_subscribe(client, DEVICE_SUBSCRIBE, 1, sub_cb, NULL);
        mqtt_publish(client, DEVICE_PUBLISH, "{\"temp\":25.0}", 13, 1, 0, pub_cb, NULL);
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
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    he = gethostbyname(HOST_NAME);
    if (he == NULL)
    {
        printf("mqtt: dns lookup failed\r\n");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    memcpy(&s_broker, he->h_addr, he->h_length);

    if (lwip_hmac_sha1((const uint8_t *)DEVICE_SECRET, strlen(DEVICE_SECRET),
                       (const uint8_t *)CONTENT, strlen(CONTENT), digest) != 0)
    {
        printf("mqtt: hmac-sha1 failed\r\n");
    }
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
