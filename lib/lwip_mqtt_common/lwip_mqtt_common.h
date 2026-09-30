/**
 * @file    lwip_mqtt_common.h
 * @brief   Shared MQTT plumbing for the Aliyun/OneNET demos.
 *
 * Keeps the connection/subscribe/publish boilerplate in one place; the app
 * supplies the topics and the UI callbacks.
 */

#ifndef LWIP_MQTT_COMMON_H
#define LWIP_MQTT_COMMON_H

#include <stdint.h>

#include "lwip/apps/mqtt.h"

/** @brief  Push a line of text to the demo display (RX area). */
typedef void (*lwip_mqtt_display_fn)(const char *text);

/** @brief  Update the connection-state line. */
typedef void (*lwip_mqtt_state_fn)(const char *text);

typedef struct
{
    const char *sub_topic;      /* topic subscribed on connect */
    const char *pub_topic;      /* topic used for the sensor samples */
    lwip_mqtt_display_fn display;
    lwip_mqtt_state_fn   state;
} lwip_mqtt_cfg_t;

/** @brief  Set the topics/callbacks (call once before connecting). */
void lwip_mqtt_init(const lwip_mqtt_cfg_t *cfg);

/** @brief  Connection callback to pass to mqtt_client_connect(). */
void lwip_mqtt_conn_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);

/** @brief  Publish one random sensor sample if connected.
 *  @return 1 if a sample was published, 0 otherwise. */
int  lwip_mqtt_poll(mqtt_client_t *client);

#endif /* LWIP_MQTT_COMMON_H */
