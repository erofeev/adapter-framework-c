/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdlib.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_console.h"
#include "adp_tcpip.h"
#include "adp_mqtt.h"
#include "adp_logging.h"

#include "app_console.h"
#include "app_mqtt.h"


void print_info(void* params)
{
    UNUSED_VAR(params);
    int interval = 60*5; // 5 minutes

    while(1) {
        adp_os_sleep(1000 * interval);
        adp_log("uptime is %d seconds", adp_os_uptime());

        // Print the whole DB
        char  *data = "db\n";
        adp_topic_publish(ADP_TOPIC_CLI_INPUT_STREAM, data, strlen(data) + 1, ADP_TOPIC_PRIORITY_HIGH);
    }
}

int app_mqtt_incoming_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_received_topic_t *topic_data = data;


    adp_log("INCOMING DATA Topic [%s], Data [%s]", topic_data->topic_name, topic_data->payload);

    return ADP_RESULT_SUCCESS;
}

int main(void) {
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("Info-print", &print_info, 128, 0, NULL);

    // Run console and subscribe on the CLI cmd execution topic
    adp_dispatcher_handle_t low_prio_dispatcher = adp_dispatcher_create(0, 25);
    adp_os_start_task("Console", &adp_console_task, 128, 0, low_prio_dispatcher);

    // Initialize TCP/IP stack & MQTT protocol
    adp_dispatcher_handle_t network_dispatcher = adp_dispatcher_create(adp_os_get_max_prio() - 3, 25);
    adp_ipnet_initialize(network_dispatcher);
    adp_mqtt_initialize(network_dispatcher);

    // Subscribe for topics we want
    adp_topic_subscribe(ADP_TOPIC_CLI_EXECUTE_CMD,     &app_cmd_handler,             "USER app_cmd_handler");
    adp_topic_subscribe(ADP_TOPIC_IPNET_IPSTATUS,      &app_net_status_handler,      "USER app_net_status_handler");
    adp_topic_subscribe(ADP_TOPIC_IPNET_CMD_STATUS,    &app_net_cmd_status_handler,  "USER app_net_cmd_status_handler");
    adp_topic_subscribe(ADP_TOPIC_MQTT_CMD_STATUS,     &app_mqtt_cmd_status_handler, "USER app_mqtt_cmd_status_handler");
    adp_topic_subscribe(ADP_TOPIC_MQTT_INCOMING_TOPIC, &app_mqtt_incoming_handler,   "USER app_mqtt_incoming_handler");

    adp_os_start();


    return EXIT_SUCCESS;
}

