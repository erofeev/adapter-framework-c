/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_logging.h"

#define ADP_DISPATCHER_TASK_NAME_TEMPLATE     "ADP#x@x"
#define ADP_DISPATCHER_TOPIC_NAME_LENGTH      12
#define ADP_DISPATCHER_CMD_NAME_LENGTH        12
#define ADP_DISPATCHER_TABLE_SIZE             50

typedef struct {
    adp_os_queue_handle_t queue_hnd;
    uint16_t          dispatcher_id;
    uint16_t               topic_id;
    uint16_t                 cmd_id;
    adp_topic_cb           *dest_cb;
    char                 topic_name[ADP_DISPATCHER_TOPIC_NAME_LENGTH];
    char                   cmd_name[ADP_DISPATCHER_TOPIC_NAME_LENGTH];
} adp_dispatcher_t;

adp_dispatcher_t dispatcher_table[ADP_DISPATCHER_TABLE_SIZE] = {0};

void adp_dispatcher_cycle(void)
{
    adp_log("-");
}

void adp_dispatcher_route_to_dst(adp_os_queue_handle_t queue, const void * const msg_ptr)
{
    adp_log("Callback is at 0x%x", msg_ptr);
}

void adp_dispatcher(void* data)
{
    const char* const     name  = adp_os_get_task_name();
    adp_os_queue_handle_t queue = (adp_os_queue_handle_t)data;
    const int             total = adp_queue_msg_total(queue);
    void * msg_ptr = NULL;

    adp_log_d("Dispatcher %s started, msg capacity %d", name, total);

    // Main dispatcher loop
    while (1) {

        if (ADP_RESULT_SUCCESS == adp_queue_receive(queue, msg_ptr, 1000 /* = 1 sec */)) {
            // Route incoming message to its destination
            adp_dispatcher_route_to_dst(queue, msg_ptr);
            adp_log_d("Queue usage: %d/%d", adp_queue_msg_waiting(queue), total);
         }
        adp_dispatcher_cycle();
    }
}

int adp_dispatcher_add(adp_os_queue_handle_t queue_hnd, int id)
{
    int i = 0;
    while ((dispatcher_table[i].queue_hnd != NULL) && (i < ADP_DISPATCHER_TABLE_SIZE)) i++;
    if (i == ADP_DISPATCHER_TABLE_SIZE) {
        adp_log_e("No space in dispatcher table");
        return ADP_RESULT_FAILED;
    }
    memset(&dispatcher_table[i], 0x00, sizeof(adp_dispatcher_t));
    dispatcher_table[i].dispatcher_id = id;
    dispatcher_table[i].queue_hnd     = queue_hnd; // Store the queue handle in the last step, considering this step atomic, now this record is valid
    adp_log_d("New dispatcher #%d is registered", id);

    return ADP_RESULT_SUCCESS;
}

int adp_dispatcher_create(uint32_t os_prio, uint32_t max_items)
{
    static int d_cnt = 0;
    uint32_t size_of_name = sizeof(ADP_DISPATCHER_TASK_NAME_TEMPLATE);

    // Create queue
    adp_os_queue_handle_t queue = adp_os_queue_create(max_items, sizeof(void*)*max_items);
    if (!queue) {
        ADP_ASSERT(0, "Unable to create a queue");
        return ADP_RESULT_FAILED;
    }

    // Assemble task name
    char *auto_name  = (char*) adp_os_malloc(size_of_name);
    if (auto_name) {
        memcpy(auto_name, ADP_DISPATCHER_TASK_NAME_TEMPLATE, size_of_name);
        auto_name[size_of_name - 2] = 0x30 + (os_prio     % 255);
        auto_name[size_of_name - 4] = 0x30 + (d_cnt % 255);
    }

    // Start dispatcher with the queue
    int result = adp_os_start_task(auto_name, &adp_dispatcher, 512, os_prio, (void*)queue);

    // Clean up, check results and exit
    if (auto_name) {
        adp_os_free(auto_name);
    }

    if (ADP_RESULT_SUCCESS == result) {
        adp_dispatcher_add(queue, d_cnt);
        d_cnt++;
        return (d_cnt - 1);
    }

    ADP_ASSERT(0, "Unable to create a dispatcher task");
    return ADP_RESULT_FAILED;
}


