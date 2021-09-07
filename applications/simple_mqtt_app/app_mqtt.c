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

void*               s_mqtt_id                = (void*)"MQTT client 1";
void*               s_mqtt_id_2              = (void*)"MQTT client 2";
void*               s_mqtt_id_3              = (void*)"MQTT client 3";
void*               s_mqtt_id_4              = (void*)"MQTT client 4";

adp_mqtt_topic_filter_t topics[] = {
        {
                .QoS = 2,
                .topic_filter = "tele",
        },
        {
                .QoS = 2,
                .topic_filter = "testxmr",
        },
};

static void do_tcp_connect(char *tcp_connect_name)
{
    adp_log("Status: starting with socket [%s]", tcp_connect_name);
    adp_ipnet_cmd_t ipnet_connect = {
               .user_ctx          = tcp_connect_name,
               .command           = ADP_IPNET_DO_TCP_CONNECT,
               .connect.port      = 1883,
               .connect.hostname  = "test.mosquitto.org" } ;
    adp_topic_publish(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_connect, sizeof(adp_ipnet_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

static void do_tcp_shutdown(char *tcp_connect_name)
{
    adp_log("Status: closing socket [%s]", tcp_connect_name);
    adp_ipnet_cmd_t ipnet_shutdown = {
               .user_ctx           = tcp_connect_name,
               .command            = ADP_IPNET_DO_TCP_SHUTDOWN } ;
    adp_topic_publish(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_shutdown, sizeof(adp_ipnet_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}


static void do_mqtt_connect(void* user_ctx, void *socket, int timeout_ms)
{
    // Connect to the broker
    adp_mqtt_cmd_t mqtt_connect = {
            .user_ctx                        = (void*)user_ctx,
            .command                         = ADP_MQTT_DO_CONNECT,
            .connect.socket                  = socket,
            .connect.clean_session           = 1,
            .connect.keep_alive_seconds      = 60,
            .connect.client_id               =  user_ctx,
            .connect.username                = "USERNAME",
            .connect.password                = "PASSWORD",
            .connect.ack_timeout_ms          = timeout_ms,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_connect, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

static void do_mqtt_subscribe(void* user_ctx)
{
    // Connect to the broker
    adp_mqtt_cmd_t mqtt_subscribe = {
            .user_ctx                        = user_ctx,
            .command                         = ADP_MQTT_DO_SUBSCRIBE,
            .subscribe.number_of_filters     = sizeof(topics)/sizeof(adp_mqtt_topic_filter_t),
            .subscribe.filters               = topics,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_subscribe, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

static void do_mqtt_disconnect(void* user_ctx)
{
    adp_mqtt_cmd_t mqtt_disconnect = {
            .user_ctx   = user_ctx,
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
            do_tcp_connect(s_mqtt_id);
            do_tcp_connect(s_mqtt_id_2);
            do_tcp_connect(s_mqtt_id_3);
            do_tcp_connect(s_mqtt_id_4);
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
    adp_log("Status: IPNET client [%s] cmd #%d executed with result %s (subcode: %d)",
            cmd_status->user_ctx,
            cmd_status->command,
            (cmd_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED",
            cmd_status->subcode,
            cmd_status->socket);

    // Process items related to our socket
    if ( (cmd_status->user_ctx != s_mqtt_id)   && (cmd_status->user_ctx != s_mqtt_id_2) &&
         (cmd_status->user_ctx != s_mqtt_id_3) && (cmd_status->user_ctx != s_mqtt_id_4) ) {
        return ADP_RESULT_SUCCESS;
    }

    switch (cmd_status->command) {
    case ADP_IPNET_DO_TCP_CONNECT:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                // Start establishing MQTT session
                do_mqtt_connect(cmd_status->user_ctx, cmd_status->socket, 2000);
            } else {
                do_tcp_shutdown(cmd_status->user_ctx);
            }
        }
        break;
    case ADP_IPNET_DO_TCP_SHUTDOWN:
        {
            do_tcp_connect(cmd_status->user_ctx);
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
    adp_log("Status: MQTT client [%s] cmd #%d executed with result %s (subcode: %d)",
            cmd_status->user_ctx,
            cmd_status->command,
            (cmd_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED",
            cmd_status->subcode);

    switch (cmd_status->command) {
    case ADP_MQTT_DO_CONNECT:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                adp_log("[%s] MQTT SUCCESFULLY CONNECTED", cmd_status->user_ctx);
                // Subscribe for topics
                do_mqtt_subscribe(cmd_status->user_ctx);
            } else {
                // Terminate the socket
                do_tcp_shutdown(cmd_status->user_ctx);
            }
        }
        break;
    case ADP_MQTT_DO_SUBSCRIBE:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                adp_log("============================================");
                adp_log("[%s] MQTT SUCCESFULLY SUBSCRIBED", cmd_status->user_ctx);
                adp_log("============================================");
            } else {
                // Send DISCONNECT, let's try to clean up everything and try again
                do_mqtt_disconnect(cmd_status->user_ctx);
            }
        }
        break;
    case ADP_MQTT_DO_DISCONNECT:
        {
             adp_log("[%s] MQTT SUCCESFULLY DISCONNECTED - DO WE WANT TO TRY AGAIN?", cmd_status->user_ctx);
             do_tcp_shutdown(cmd_status->user_ctx);
        }
        break;
    default:
        break;
    }

    return ADP_RESULT_SUCCESS;
}

