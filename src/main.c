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
#include "adp_logging.h"


void test_publisher_1(void* params)
{
    int interval = 1;


    while(1) {
        adp_os_sleep(1000 * interval);
        adp_log("uptime is %d seconds", adp_os_uptime());
        char *data        = "Test data to transfer from test_publisher_1";
        int   data_length = strlen(data) + 1;
        adp_topic_publish(0x00000010, data, data_length, ADP_TOPIC_PRIORITY_HIGH);
    }
}

int test_subscriber_1(uint16_t topic_id, void* data, uint32_t len)
{
    adp_log("Subscriber received data [%s]", data);
    return ADP_RESULT_SUCCESS;
}

int test_subscriber_2(uint16_t topic_id, void* data, uint32_t len)
{
    adp_log("Get all messages: [%s]", data);
    return ADP_RESULT_SUCCESS;
}

int main(void) {
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("Info-print", &test_publisher_1, 512, 6, NULL);

    // Example of first dispatcher creation with task prio 5 and items number 25
    adp_dispatcher_handle_t dispatcher_1 = adp_dispatcher_create(5, 25);

    // Example of second dispatcher creation with task prio 4 and items number 35
    adp_dispatcher_handle_t dispatcher_2 = adp_dispatcher_create(4, 35);

    // Example of third dispatcher creation with the same configuration as above
    adp_dispatcher_handle_t dispatcher_3 = adp_dispatcher_create(4, 45);

    // Example of adding few topics
    uint32_t topic_id   = 0x00000010;
    adp_topic_register(dispatcher_1, topic_id, "Temperature");
    adp_topic_register(dispatcher_1, topic_id | 0x1, "getCurrent");
    adp_topic_register(dispatcher_1, topic_id | 0x2, "getPrev");
    adp_topic_register(dispatcher_1, topic_id | 0x3, "getAverage");

    adp_topic_register(dispatcher_2, 0x00000100, "Sensors");
    adp_topic_register(dispatcher_3, 0x00000200, "Connectivity");

    adp_topic_register(dispatcher_1, 0x00000000, "Zero");

    // Example of publish
    char *data        = "Test data to transfer";
    int   data_length = strlen(data) + 1;
    adp_topic_publish(topic_id, data, data_length, ADP_TOPIC_PRIORITY_HIGH);

   // adp_topic_publish(0x0, data, data_length, ADP_TOPIC_PRIORITY_HIGH);

    // Example of subscribing
    adp_topic_subscribe(0x000000FF, &test_subscriber_1, "test_subscriber_1");
    adp_topic_subscribe(0xFFFFFFFF, &test_subscriber_2, "catch_everything");

    adp_dispatcher_db_print();

    adp_os_start();

    return EXIT_SUCCESS;
}
