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
    int interval = 60*1; // 1 minute
    char  *data;

    while(1) {
        adp_os_sleep(1000 * interval);
        adp_log("GenInfo: uptime is %d seconds", adp_os_uptime());

        // Print the whole DB
        data = "db\n";
        adp_topic_publish(ADP_TOPIC_CLI_INPUT_STREAM, data, strlen(data) + 1, ADP_TOPIC_PRIORITY_HIGH);

        // Print the memory trace DB
        data = "mem\n";
        adp_topic_publish(ADP_TOPIC_CLI_INPUT_STREAM, data, strlen(data) + 1, ADP_TOPIC_PRIORITY_HIGH);
    }
}

// Handling: Net is UP or DOWN
int net_status_handler(uint32_t topic_id, void* data, uint32_t len)
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
            // Nothing to do
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


int main(void) {
    adp_log("Build date [%s %s]", __DATE__, __TIME__);
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("APP Info-print", &print_info, 128, 0, NULL);

    // Run console and subscribe on the CLI cmd execution topic
    adp_dispatcher_handle_t low_prio_dispatcher = adp_dispatcher_create("CLI", 0, 80);
    adp_os_start_task("APP Console", &adp_console_task, 128, 0, low_prio_dispatcher);
    adp_topic_subscribe(ADP_TOPIC_CLI_EXECUTE_CMD, &app_cmd_handler, "USER app_cmd_handler");

    // Initialize TCP/IP stack
    adp_dispatcher_handle_t network_dispatcher = adp_dispatcher_create("IPNET", adp_os_get_max_prio() - 2, 120);
    adp_ipnet_initialize(network_dispatcher);

    // Initialize MQTT
    adp_dispatcher_handle_t mqtt_dispatcher    = adp_dispatcher_create("MQTT",  adp_os_get_max_prio() - 3, 40);
    adp_mqtt_initialize(mqtt_dispatcher);

    // Subscribe
    adp_topic_subscribe(ADP_TOPIC_IPNET_IPSTATUS, &net_status_handler, "USER Net Up/Down");

    adp_os_start();

    return EXIT_SUCCESS;
}

