/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdint.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_logging.h"
#include "adp_tcpip.h"
#include "adp_mqtt.h"

adp_socket_t        s_tcp_socket_mosquitto = NULL;
void*               s_mqtt_id              = (void*)1;
adp_mqtt_topic_filter_t topics[] = {
        {
                .QoS = 0,
                .topic_filter = "tele",
        },
        {
                .QoS = 2,
                .topic_filter = "testxmr",
        },
};


static void do_tcp_connect()
{
    // Clean socket
    if (!s_tcp_socket_mosquitto) {
        // Create a new tcp socket and try to connect to the server
        s_tcp_socket_mosquitto = adp_ipnet_socket_alloc(ADP_SOCKET_TCP);
        ADP_ASSERT(s_tcp_socket_mosquitto, "Fatal problem: unable to create socket");
    }
    adp_log("Status: starting with socket - 0x%x", s_tcp_socket_mosquitto);
    adp_ipnet_cmd_t ipnet_connect = {
               .command           = ADP_IPNET_DO_TCP_CONNECT,
               .socket            = s_tcp_socket_mosquitto,
               .connect.port      = 1883,
               .connect.hostname  = "test.mosquitto.org" } ;
    adp_topic_publish(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_connect, sizeof(adp_ipnet_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}


static void do_mqtt_connect(void *socket, int timeout_ms)
{
    // Connect to the broker
    adp_mqtt_cmd_t mqtt_connect = {
            .user_id                         = (void*)s_mqtt_id,
            .command                         = ADP_MQTT_DO_CONNECT,
            .connect.socket                  = socket,
            .connect.clean_session           = 1,
            .connect.keep_alive_seconds      = 60,
            .connect.client_id               = "CLIENT_ID",
            .connect.username                = "USERNAME",
            .connect.password                = "PASSWORD",
            .connect.ack_timeout_ms          = timeout_ms,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_connect, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

static void do_mqtt_subscribe()
{
    // Connect to the broker
    adp_mqtt_cmd_t mqtt_subscribe = {
            .user_id                        = (void*)s_mqtt_id,
            .command                         = ADP_MQTT_DO_SUBSCRIBE,
            .subscribe.number_of_filters     = sizeof(topics)/sizeof(adp_mqtt_topic_filter_t),
            .subscribe.filters               = topics,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_subscribe, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

static void do_mqtt_disconnect()
{
    adp_mqtt_cmd_t mqtt_disconnect = {
            .user_id    = (void*)s_mqtt_id,
            .command    = ADP_MQTT_DO_DISCONNECT,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_disconnect, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

// Handling: Net is UP or DOWN
int app_net_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    static adp_ipnet_status_t prev_status = ADP_IPNET_STACK_NA;
    adp_ipnet_status_t status = *(adp_ipnet_status_t*)data;

    if (status == prev_status) {
        // DHCP renew happened
        return ADP_RESULT_SUCCESS;
    }
    adp_log("Status: Network is %s", (status == ADP_IPNET_STACK_DOWN) ? "down" : "up");

    switch (status) {
    case ADP_IPNET_STACK_UP:
        {
            // Create a tcp socket and try to connect to the server
            do_tcp_connect();
        }
        break;
    case ADP_IPNET_STACK_DOWN:
        {
            // Nothing to do
        }
        break;
    default:
        break;
    }

    prev_status = status;
    return ADP_RESULT_SUCCESS;
}

// Handling: Socket connection success or not
int app_net_cmd_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_ipnet_cmd_status_t *cmd_status = (adp_ipnet_cmd_status_t*)data;
    adp_log("Status: IPNET cmd #%d executed with result %s (subcode: %d socket is 0x%x)",
            cmd_status->command,
            (cmd_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED",
            cmd_status->subcode,
            cmd_status->socket);

    // Process items related to our socket
    if (cmd_status->socket != s_tcp_socket_mosquitto) {
        return ADP_RESULT_SUCCESS;
    }

    switch (cmd_status->command) {
    case ADP_IPNET_DO_TCP_CONNECT:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                // Start establishing MQTT session
                do_mqtt_connect(cmd_status->socket, 1000);
            } else {
                // Create a tcp socket and try to connect to the server again
                do_tcp_connect();
            }
        }
        break;
    default:
        break;
    }

    return ADP_RESULT_SUCCESS;
}


// Handling: MQTT session
int app_mqtt_cmd_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_cmd_status_t *cmd_status = (adp_mqtt_cmd_status_t*)data;
    adp_log("Status: MQTT cmd #%d executed with result %s (subcode: %d userId 0x%x)",
            cmd_status->command,
            (cmd_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED",
            cmd_status->subcode,
            cmd_status->user_id);

    switch (cmd_status->command) {
    case ADP_MQTT_DO_CONNECT: // FIXME
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                adp_log("MQTT SUCCESFULLY CONNECTED");
                // Subscribe for topics
                do_mqtt_subscribe();
            } else {
                // Send DISCONNECT, let's try to clean up everything and try again
                do_mqtt_disconnect();
            }
        }
        break;
    case ADP_MQTT_DO_SUBSCRIBE:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                adp_log("MQTT SUCCESFULLY SUBSCRIBED");
            } else {
                // Send DISCONNECT, let's try to clean up everything and try again
                do_mqtt_disconnect();
            }
        }
        break;
    case ADP_MQTT_DO_DISCONNECT:
        {
             adp_log("MQTT SUCCESFULLY DISCONNECTED - DO WE WANT TO TRY AGAIN? In 1 second...");
             adp_ipnet_socket_free(s_tcp_socket_mosquitto);
             s_tcp_socket_mosquitto = NULL;
             adp_os_sleep(1000);
             do_tcp_connect();
        }
        break;
    default:
        break;
    }

    return ADP_RESULT_SUCCESS;
}
