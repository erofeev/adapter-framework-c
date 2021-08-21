/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_console.h"
#include "adp_logging.h"


void test_subpub(void* params)
{
    int interval = 15; // 15 seconds

    while(1) {
        adp_os_sleep(1000 * interval);
        adp_log("uptime is %d seconds", adp_os_uptime());
        char *data        = "db\n";
        int   data_length = strlen(data) + 1;
        adp_topic_publish(ADP_TOPIC_SYSTEM_CLI_INPUT_STREAM, data, data_length, ADP_TOPIC_PRIORITY_HIGH);
    }
}

int app_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_log("APP CLI CMD executor, cmd = [%s]", data);

    if (strcmp("db", data) == 0) {
        adp_dispatcher_db_print(NULL);
    }

    return ADP_RESULT_SUCCESS;
}

int main(void) {
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("Info-print", &test_subpub, 512, 6, NULL);


    // Example of first dispatcher creation with task prio 5 and items number 25
    adp_dispatcher_handle_t dispatcher_1 = adp_dispatcher_create(5, 25);
    adp_dispatcher_handle_t dispatcher_2 = adp_dispatcher_create(4, 35);
    adp_dispatcher_handle_t dispatcher_3 = adp_dispatcher_create(4, 45);

    // Example of adding few topics
    uint32_t topic_id   = 0x00000010;
    adp_topic_register(dispatcher_1, topic_id, "Temperature");
    adp_topic_register(dispatcher_1, topic_id | 0x1, "getCurrent");
    adp_topic_register(dispatcher_1, topic_id | 0x2, "getPrev");
    adp_topic_register(dispatcher_1, topic_id | 0x3, "getAverage");
    adp_topic_register(dispatcher_2, 0x00000100, "Sensors");
    adp_topic_register(dispatcher_3, 0x00000200, "Connectivity");
    adp_topic_register(dispatcher_1, 0x10000000, "");

    // Run console
    adp_dispatcher_handle_t system_dispatcher = adp_dispatcher_create(0, 25);
    adp_os_start_task("Console", &adp_console_task, 128, 0, system_dispatcher);

    // Add app handler for receiving CLI commands requested for the execution
    adp_topic_subscribe(ADP_TOPIC_SYSTEM_CLI_EXECUTE_CMD, &app_cmd_handler, "App CMD handler");

    adp_os_start();

    return EXIT_SUCCESS;
}
