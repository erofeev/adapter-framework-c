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
    adp_dispatcher_handle_t       handle;
    uint16_t               dispatcher_id;
    uint16_t                    topic_id;
    uint16_t                      cmd_id;
    adp_topic_cb                *dest_cb;
    char                      topic_name[ADP_DISPATCHER_TOPIC_NAME_LENGTH];
    char                        cmd_name[ADP_DISPATCHER_TOPIC_NAME_LENGTH];
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

int adp_dispatcher_add(adp_dispatcher_handle_t handle, int id)
{
    int i = 0;
    while ((dispatcher_table[i].handle != NULL) && (i < ADP_DISPATCHER_TABLE_SIZE)) i++;
    if (i == ADP_DISPATCHER_TABLE_SIZE) {
        adp_log_e("Unable to create dispatcher: No space left in DB");
        return ADP_RESULT_FAILED;
    }
    memset(&dispatcher_table[i], 0x00, sizeof(adp_dispatcher_t));
    dispatcher_table[i].dispatcher_id = id;
    dispatcher_table[i].handle        = handle;
    adp_log_d("New dispatcher #%d is registered", id);

    return ADP_RESULT_SUCCESS;
}

int adp_topic_register(adp_dispatcher_handle_t dispatcher_hnd, uint16_t topic_id, const char* topic_name, uint16_t cmd_id, const char* cmd_name, adp_topic_cmd_prio_t prio)
{
    if ((!dispatcher_hnd) || (!topic_name) || (!cmd_name)) {
        ADP_ASSERT(0, "Null parameter specified");
        return ADP_RESULT_FAILED;
    }

    // - Validate the handle (it should be in the DB)
    // - Find an empty slot for new record
    // - Check that there is no such topic+cmd in the DB
    uint16_t dispatcher_id;
    bool     is_handle_in_the_table = false;
    int      empty_slot_id          = -1;
    bool     topic_cmd_unique       = true;
    for (int i = 0; i < ADP_DISPATCHER_TABLE_SIZE; ++i) {
        if (dispatcher_table[i].handle == NULL) {
            empty_slot_id = i;
            continue;
        }
        if (dispatcher_table[i].handle == dispatcher_hnd) {
            is_handle_in_the_table = true;
            dispatcher_id = dispatcher_table[i].dispatcher_id;
        }
        if ((dispatcher_table[i].topic_id == topic_id) && (dispatcher_table[i].cmd_id == cmd_id)) {
            topic_cmd_unique = false;
        }
    }

    // No empty slot is found
    if (empty_slot_id == -1) {
        adp_log_e("Key [Topic:0x%x & cmd:0x%x]: No space left in DB", topic_id, cmd_id);
        return ADP_RESULT_FAILED;
    }

    // The handle is not found
    if (is_handle_in_the_table == false) {
        adp_log_e("Handler 0x%x not found in DB", dispatcher_hnd);
        return ADP_RESULT_FAILED;
    }

    if (topic_cmd_unique == false) {
        adp_log_e("Key [Topic:0x%x & cmd:0x%x] is already registered", topic_id, cmd_id);
        return ADP_RESULT_FAILED;
    }

    // Add new record
    dispatcher_table[empty_slot_id].handle        = dispatcher_hnd;
    dispatcher_table[empty_slot_id].dispatcher_id = dispatcher_id;
    dispatcher_table[empty_slot_id].topic_id      = topic_id;
    dispatcher_table[empty_slot_id].cmd_id        = cmd_id;
    snprintf(dispatcher_table[empty_slot_id].topic_name, ADP_DISPATCHER_TOPIC_NAME_LENGTH, "%s", topic_name);
    snprintf(dispatcher_table[empty_slot_id].cmd_name  , ADP_DISPATCHER_CMD_NAME_LENGTH  , "%s", cmd_name);
    adp_log_d("Dispatcher #%d, new topic 0x%x[%s] and cmd 0x%x[%s] registered",
            dispatcher_id, topic_id, topic_name, cmd_id, cmd_name);

    return ADP_RESULT_SUCCESS;
}

adp_dispatcher_handle_t adp_dispatcher_create(uint32_t os_prio, uint32_t max_items)
{
    static int d_cnt = 0;
    uint32_t size_of_name = sizeof(ADP_DISPATCHER_TASK_NAME_TEMPLATE);

    // Create queue
    adp_os_queue_handle_t queue = adp_os_queue_create(max_items, sizeof(void*)*max_items);
    if (!queue) {
        ADP_ASSERT(0, "Unable to create a queue");
        return (adp_dispatcher_handle_t)NULL;
    }

    // Construct task name
    char *auto_name  = (char*) adp_os_malloc(size_of_name);
    if (auto_name) {
        memcpy(auto_name, ADP_DISPATCHER_TASK_NAME_TEMPLATE, size_of_name);
        auto_name[size_of_name - 2] = 0x30 + (os_prio % 255);
        auto_name[size_of_name - 4] = 0x30 + (d_cnt   % 255);
    }

    // Start dispatcher with the queue
    int result = adp_os_start_task(auto_name, &adp_dispatcher, 512, os_prio, (void*)queue);

    // Clean up, check results and exit
    if (auto_name) {
        adp_os_free(auto_name);
    }

    if (ADP_RESULT_SUCCESS == result) {
        if (ADP_RESULT_SUCCESS == adp_dispatcher_add((adp_dispatcher_handle_t)queue, d_cnt)) {
            d_cnt++;
            return (adp_dispatcher_handle_t)queue;
        }
    }

    ADP_ASSERT(0, "Unable to create a dispatcher");
    return (adp_dispatcher_handle_t)NULL;
}


