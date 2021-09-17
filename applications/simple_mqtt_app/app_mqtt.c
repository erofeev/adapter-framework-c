/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdlib.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_logging.h"
#include "adp_mqtt_agent.h"

adp_mqtt_client_t client_template = {
        .name                = "",
        .hostname            = "test.mosquitto.org",// IP: 5.196.95.208
        .ip_octet1           = 5,
        .ip_octet2           = 196,
        .ip_octet3           = 95,
        .ip_octet4           = 208,
        .port                = 1883,
        .clean_session       = 1,
        .keep_alive_seconds  = 100,
        .client_id           = "",
        .username            = "TEST",
        .password            = "TEST",
        .ack_timeout_ms      = 2000,
        .number_of_topics    = 2,
        .topics = {
                {
                        .QoS = 0,
                        .topic_filter = "test",
                },
                {
                        .QoS = 2,
                        .topic_filter = "testxmr",
                },
        },

};

int app_mqtt_incoming_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_received_topic_t *topic_data = data;
    adp_mqtt_client_t *client = (adp_mqtt_client_t *)topic_data->user_ctx;

    adp_log("Client [%s] INCOMING DATA Topic [%s], Data [%s]", client->name, &topic_data->buffer[0], &topic_data->buffer[topic_data->topic_name_size]);

    return ADP_RESULT_SUCCESS;
}

void start_mqtt_client(const char *client_name)
{
    static int init = 0;
    if (!client_name) {
        adp_log_e("Client name is NULL. Do nothing");
        return;
    }
    if (!init) {
        adp_topic_subscribe(ADP_TOPIC_MQTT_INCOMING_TOPIC, &app_mqtt_incoming_handler, "USER Print MQTT topics");
        init = 1;
    }

    // Clone client_1 which is used as a template for new clients in this example project
    adp_mqtt_client_t *new_client = adp_os_malloc(sizeof(adp_mqtt_client_t) + sizeof(adp_mqtt_topic_filter_t) * client_template.number_of_topics);
    ADP_ASSERT(new_client,"No memory left");
    memcpy(new_client, &client_template, sizeof(adp_mqtt_client_t) + sizeof(adp_mqtt_topic_filter_t) * client_template.number_of_topics);

    // Clone name and client_id
    new_client->name = adp_os_malloc(strlen(client_name) + 1);
    ADP_ASSERT(new_client->name,"No memory left");
    memcpy(new_client->name, client_name, strlen(client_name) + 1);
    new_client->client_id = new_client->name;
    new_client->keep_alive_seconds = 30 + adp_os_rand() % 360;

    adp_mqtt_agent_start(new_client);

}

