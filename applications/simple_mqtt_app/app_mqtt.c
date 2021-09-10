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

adp_mqtt_client_t client_1 = {
        .name                = "Client#1",
        .hostname            = "test.mosquitto.org",
        .port                = 1883,
        .clean_session       = 1,
        .keep_alive_seconds  = 60,
        .client_id           = "Client 0x00",
        .username            = "TEST",
        .password            = "TEST",
        .ack_timeout_ms      = 2000,
        .number_of_topics    = 2,
        .topics = {
                {
                        .QoS = 0,
                        .topic_filter = "tele",
                },
                {
                        .QoS = 2,
                        .topic_filter = "testxmr",
                },
        },

};

void start_mqtt_client(const char *client_name)
{
    if (!client_name) {
        adp_log_e("Client name is NULL. Do nothing");
        return;
    }

    // Clone client_1 which is used as a template for new clients in this example project
    adp_mqtt_client_t *new_client = adp_os_malloc(sizeof(adp_mqtt_client_t) + sizeof(adp_mqtt_topic_filter_t) * client_1.number_of_topics);
    ADP_ASSERT(new_client,"No memory left");
    memcpy(new_client, &client_1, sizeof(client_1));

    // Clone name and client_id
    new_client->name = adp_os_malloc(strlen(client_name) + 1);
    ADP_ASSERT(new_client->name,"No memory left");
    memcpy(new_client->name, client_name, strlen(client_name) + 1);
    new_client->client_id = new_client->name;

    // Clone topics
    memcpy(new_client->topics, client_1.topics, sizeof(adp_mqtt_topic_filter_t) * client_1.number_of_topics);

    adp_mqtt_agent_start(new_client);
}

