/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_logging.h"


void task_print_info(void* params)
{
    int interval = 1;

    while(1) {
        adp_os_sleep(1000 * interval);
        adp_log("uptime is %d seconds", adp_os_uptime());
    }
}


int main(void) {
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("Info-print", &task_print_info, 512, 2, NULL);

    // Example of first dispatcher creation with task prio 5 and items number 25
    int d1_prio  = 5;
    int d1_items = 25;
    int d1_id = adp_dispatcher_create(d1_prio, d1_items);

    // Example of second dispatcher creation with task prio 4 and items number 35
    int d2_id = adp_dispatcher_create(4, 35);

    // Example of third dispatcher creation with the same configuration as above
    int d3_id = adp_dispatcher_create(4, 35);

    (void)(d1_id);
    (void)(d2_id);
    (void)(d3_id);

  //  adp_topic_create(dispatcher_id, topic_id, cmd_id, prio);
  //  adp_topic_is_(topic_id);
  //  adp_topic_get_list(dispatcher_id);
  //  result = adp_topic_publish(topic_id, cmd_id, data, data_length, prio, timeout); // result ok, result not ok
  //  adp_topic_subscribe (topic_id_mask, cmd_mask, callback);    // result ok, result not ok

  //

    adp_os_start();

    return EXIT_SUCCESS;
}
