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
    adp_net_tcpip_status_t status = *(adp_net_tcpip_status_t*)data;

    adp_log("Network is %s", (status == ADP_NET_TCPIP_STACK_DOWN) ? "down" : "up");

    switch (status) {
    case ADP_NET_TCPIP_STACK_UP:
        {
            adp_log("IP stack is up -> initialize MQTT");
            adp_mqtt_cmd_t mqtt_execute;
            mqtt_execute.command = ADP_MQTT_DO_INIT;
            adp_topic_publish(ADP_TOPIC_SYSTEM_MQTT_EXECUTE_CMD, &mqtt_execute, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
        }
        break;
    case ADP_NET_TCPIP_STACK_DOWN:
        {
            adp_log("IP stack is down");
        }
        break;
    default:
        break;
    }

    return ADP_RESULT_SUCCESS;
}


int app_mqtt_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_status_t *mqtt_status = (adp_mqtt_status_t*)data;
    adp_log("MQTT status of cmd#%d: %s subcode: %d", mqtt_status->command,
            (mqtt_status->status == ADP_RESULT_SUCCESS) ? "SUCESS" : "FAILED", mqtt_status->subcode);

    return ADP_RESULT_SUCCESS;
}
