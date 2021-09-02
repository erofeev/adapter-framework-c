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

adp_socket_t        tcp_socket_mosquitto = NULL;
adp_mqtt_session_t *mqtt_session         = NULL;

static void do_tcp_connect()
{
    // Clean socket
    if (tcp_socket_mosquitto) {
        adp_ipnet_socket_free(tcp_socket_mosquitto);
    }
    // Create a new tcp socket and try to connect to the server
    tcp_socket_mosquitto = adp_ipnet_socket_alloc(ADP_SOCKET_TCP);
    if (!tcp_socket_mosquitto) {
        adp_log_e("Fatal problem: unable to create socket");
        ADP_ASSERT(tcp_socket_mosquitto, "General fault");
    }
    adp_ipnet_cmd_t ipnet_connect = {
               .command           = ADP_IPNET_DO_TCP_CONNECT,
               .connect.socket    = tcp_socket_mosquitto,
               .connect.port      = 1883,
               .connect.hostname  = "test.mosquitto.org" } ;
    adp_topic_publish(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_connect, sizeof(adp_ipnet_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

static void do_mqtt_connect(int timeout_ms)
{
    if (mqtt_session) {
        adp_mqtt_session_free(mqtt_session);
    }
    // Create a new tcp socket and try to establish MQTT connection to the broker
    mqtt_session = adp_mqtt_session_alloc(tcp_socket_mosquitto);
    ADP_ASSERT(mqtt_session, "Fatal problem: unable to create MQTT session");

    // Connect to the broker
    adp_mqtt_cmd_t mqtt_connect = {
            .command                         = ADP_MQTT_DO_CONNECT,
            .connect.session_id              = mqtt_session,
            .connect.clean_session           = 1,
            .connect.keep_alive_seconds      = 60,
            .connect.client_id               = "CLIENT_ID",
            .connect.username                = "USERNAME",
            .connect.password                = "PASSWORD",
            .connect.ack_timeout_ms          = timeout_ms,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_connect, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
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
            adp_ipnet_socket_free(tcp_socket_mosquitto);
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

    // Process only for our socket
    if (cmd_status->socket != tcp_socket_mosquitto) {
        return ADP_RESULT_SUCCESS;
    }

    switch (cmd_status->command) {
    case ADP_IPNET_DO_TCP_CONNECT:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                // Start establishing MQTT session
                do_mqtt_connect(50);
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
    static int timeout_ms = 100;
    adp_mqtt_cmd_status_t *cmd_status = (adp_mqtt_cmd_status_t*)data;
    adp_log("Status: MQTT cmd #%d executed with result %s (subcode: %d sessionId 0x%x)",
            cmd_status->command,
            (cmd_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED",
            cmd_status->subcode,
            cmd_status->session_id);

    // MQTT init is done, try to connect to the broker
    if (cmd_status->command == ADP_MQTT_DO_CONNECT) {
        if (cmd_status->status != ADP_RESULT_SUCCESS) {
            // Try again
            do_mqtt_connect(timeout_ms);
            timeout_ms += 50;
            timeout_ms = timeout_ms % 3000; // not greater than 3 seconds
        } else {
            adp_log("MQTT SUCCESFULLY CONNECTED");
        }
    } else {
        // Nothing to do
    }

    return ADP_RESULT_SUCCESS;
}
