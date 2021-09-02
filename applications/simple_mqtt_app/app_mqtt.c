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


int app_net_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    static adp_ipnet_status_t prev_status = ADP_IPNET_STACK_NA;
    adp_ipnet_status_t status = *(adp_ipnet_status_t*)data;

    if (status == prev_status) {
        return ADP_RESULT_SUCCESS;
    }
    adp_log("Status: Network is %s", (status == ADP_IPNET_STACK_DOWN) ? "down" : "up");

    switch (status) {
    case ADP_IPNET_STACK_UP:
        {
            adp_mqtt_cmd_t mqtt_init = { .command = ADP_MQTT_DO_INIT };
           // adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_init, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);


            /// REMOVE ME
            adp_ipnet_cmd_t ipnet_connect = { .command = ADP_IPNET_DO_TCP_CONNECT,
                       .connect.port      = 1883,
                       .connect.hostname = "test.mosquitto.org" } ;
            adp_topic_publish(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_connect, sizeof(adp_ipnet_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
            ///
        }
        break;
    case ADP_IPNET_STACK_DOWN:
        {
            // Nothing so far
        }
        break;
    default:
        break;
    }

    prev_status = status;
    return ADP_RESULT_SUCCESS;
}


int app_net_cmd_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_ipnet_cmd_status_t *cmd_status = (adp_ipnet_cmd_status_t*)data;
    adp_log("Status: IPNET cmd #%d executed with result %s subcode: %d  socket id is 0x%x",
            cmd_status->command,
            (cmd_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED",
            cmd_status->subcode,
            cmd_status->socket_id);

    return ADP_RESULT_SUCCESS;
}


int app_mqtt_cmd_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_cmd_status_t *cmd_status = (adp_mqtt_cmd_status_t*)data;
    adp_log("Status: MQTT cmd #%d executed with result %s subcode: %d  session id is 0x%x",
            cmd_status->command,
            (cmd_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED",
            cmd_status->subcode,
            cmd_status->session_id);

    // MQTT init is done, try to connect to the broker
    if ( (cmd_status->command == ADP_MQTT_DO_INIT) && (cmd_status->status == ADP_RESULT_SUCCESS) ) {
        // Connect to the broker
        adp_mqtt_cmd_t mqtt_connect = {
                .command                         = ADP_MQTT_DO_CONNECT,
                .connect.session_id              = cmd_status->session_id,
                .connect.clean_session           = 1,
                .connect.keep_alive_seconds      = 60,
                .connect.client_id               = "CLIENT_ID",
                .connect.username                = "USERNAME",
                .connect.password                = "PASSWORD",
        };
        adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_connect, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
    } else {
        // Nothing to do
    }

    return ADP_RESULT_SUCCESS;
}
