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


int main(void) {
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("Info-print", &print_info, 128, 0, NULL);

    // Run console and subscribe on the CLI cmd execution topic
    adp_dispatcher_handle_t system_dispatcher = adp_dispatcher_create(0, 25);
    adp_os_start_task("Console", &adp_console_task, 128, 0, system_dispatcher);
    adp_topic_subscribe(ADP_TOPIC_CLI_EXECUTE_CMD, &app_cmd_handler, "App CMD handler");

    // Run TCP/IP stack
    adp_dispatcher_handle_t network_dispatcher = adp_dispatcher_create(3, 5);
    adp_ipnet_initialize(network_dispatcher);
    adp_topic_subscribe(ADP_TOPIC_IPNET_IPSTATUS, &app_net_status_handler, "App NET handler");

    // Run MQTT client
    adp_dispatcher_handle_t mqtt_dispatcher = adp_dispatcher_create(3, 25);
    adp_mqtt_initialize(mqtt_dispatcher);
    adp_topic_subscribe(ADP_TOPIC_MQTT_STATUS, &app_mqtt_status_handler, "App MQTT handler");

    adp_os_start();

    return EXIT_SUCCESS;
}

