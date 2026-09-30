/**
 * @file    lwip_mqtt_common.c
 * @brief   Shared MQTT plumbing for the Aliyun/OneNET demos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip_mqtt_common.h"

static lwip_mqtt_cfg_t s_cfg;
static uint8_t         s_connected;

static void mqtt_pub_cb(void *arg, err_t err)
{
    (void)arg;

    if (err == ERR_OK && s_cfg.display != NULL)
    {
        s_cfg.display("publish ok");
    }
}

static void mqtt_sub_cb(void *arg, err_t err)
{
    (void)arg;
    (void)err;
}

static void mqtt_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    (void)arg;
    (void)data;
    (void)len;
    (void)flags;
}

static void mqtt_inpub_cb(void *arg, const char *topic, u32_t tot_len)
{
    (void)arg;
    (void)tot_len;

    if (s_cfg.display != NULL)
    {
        s_cfg.display(topic != NULL ? topic : "");
    }
}

void lwip_mqtt_init(const lwip_mqtt_cfg_t *cfg)
{
    if (cfg != NULL)
    {
        s_cfg = *cfg;
    }
    s_connected = 0U;
}

void lwip_mqtt_conn_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    (void)arg;

    if (status == MQTT_CONNECT_ACCEPTED)
    {
        if (s_cfg.state != NULL)
        {
            s_cfg.state("State:Connection Successful");
        }

        mqtt_set_inpub_callback(client, mqtt_inpub_cb, mqtt_data_cb, NULL);

        if (s_cfg.sub_topic != NULL)
        {
            mqtt_subscribe(client, s_cfg.sub_topic, 1, mqtt_sub_cb, NULL);
        }

        s_connected = 1U;
    }
    else
    {
        if (s_cfg.state != NULL)
        {
            s_cfg.state("State:Disconnect");
        }

        s_connected = 0U;
    }
}

int lwip_mqtt_poll(mqtt_client_t *client)
{
    char payload[128];
    int  temp;
    int  humi10;

    if (client == NULL || s_connected == 0U || !mqtt_client_is_connected(client))
    {
        return 0;
    }

    temp = 30 + rand() % 10 + 1;
    humi10 = 548 + (rand() % 100);   /* 54.8 + rand; tenths formatted by hand */

    sprintf(payload,
            "{\"params\":{\"CurrentTemperature\":+%d.0,\"RelativeHumidity\":%d.%d},"
            "\"method\":\"thing.event.property.post\"}",
            temp, humi10 / 10, humi10 % 10);

    mqtt_publish(client, s_cfg.pub_topic, payload, strlen(payload), 1, 0, mqtt_pub_cb, NULL);

    if (s_cfg.display != NULL)
    {
        s_cfg.display(payload);
    }

    return 1;
}
