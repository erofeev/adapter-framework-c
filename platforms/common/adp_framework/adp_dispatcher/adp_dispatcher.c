/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <string.h>

#include "adp_dispatcher.h"
#include "adp_logging.h"


#if !ADP_DISPATCHER_MODULE_DEBUG
    #undef  adp_log_d
    #undef  adp_log_dd
    #define adp_log_d(...)
    #define adp_log_dd(...)
#endif


typedef struct {
    adp_dispatcher_handle_t       handle;
    uint16_t               dispatcher_id;
    uint32_t                    topic_id;
    char                      topic_name[ADP_DISPATCHER_TOPIC_NAME_SIZE];
} adp_dispatcher_t;

typedef struct {
    uint32_t                topic_target;
    adp_topic_cb                *dest_cb;
    char                    dest_cb_name[ADP_DISPATCHER_SUBSCRIBER_NAME_SIZE];
} adp_subscriber_t;

typedef struct {
    uint32_t    topic_id;
    uint32_t    length;
    void       *data;
} adp_generic_msg_t;


adp_dispatcher_t dispatcher_table[ADP_DISPATCHER_TABLE_SIZE] = {0};
adp_subscriber_t subscriber_table[ADP_SUBSCRIBER_TABLE_SIZE] = {0};


ADP_WEAK
void adp_dispatcher_cycle(int busy, adp_os_queue_handle_t queue)
{
    adp_os_sleep(10);
}


void adp_dispatcher_db_print(adp_dispatcher_handle_t dispatcher_handle)
{
    adp_log("=========================================================================" );
    adp_log("## | Dis.ID |   Handle   |  Topic.ID  |   Topic.NAME         | Subscriber  " );
    adp_log("-------------------------------------------------------------------------" );
    int counter = 1;
    for (int i = 0; i < ADP_DISPATCHER_TABLE_SIZE; ++i) {
        if (dispatcher_handle) { // Print records for all dispatchers (all the table) if NULL is specified
            if ((!dispatcher_table[i].handle) || (dispatcher_table[i].handle != dispatcher_handle)) {
                continue;
            }
        }
        uint32_t id       = dispatcher_table[i].dispatcher_id;
        adp_dispatcher_handle_t handle = dispatcher_table[i].handle;
        uint32_t topic_id = dispatcher_table[i].topic_id;
        char  *topic_name = dispatcher_table[i].topic_name;
        int  is_sub_found = 0;
        for (int j = 0; j < ADP_SUBSCRIBER_TABLE_SIZE; ++j) {
            if (!subscriber_table[j].dest_cb) {
                continue;
            }
            if (!topic_id) {
                continue;
            }
            if (subscriber_table[j].topic_target == topic_id) {
                is_sub_found = 1;
                adp_log("%02d |   0x%02d | 0x%08x | 0x%08x | % 20s -> %s",
                        counter++, id, handle, topic_id, topic_name, subscriber_table[j].dest_cb_name);
            }
        }
        if (is_sub_found == 0) {
            adp_log("%02d |   0x%02d | 0x%08x | 0x%08x | % 20s |", counter++, id, handle, topic_id, topic_name);
        }
        adp_log("-------------------------------------------------------------------------" );
    }
}


adp_result_t adp_topic_register(adp_dispatcher_handle_t dispatcher_hnd, uint32_t topic_id, const char* topic_name)
{
    if ( (!dispatcher_hnd) || (!topic_name) || (!topic_id) ) {
        ADP_ASSERT(0, "Invalid parameter specified");
        return ADP_RESULT_INVALID_PARAMETER;
    }

    // - Validate the handle (it should be in the DB)
    // - Find an empty slot for new record
    // - Check that there is no such topic+cmd in the DB
    uint16_t dispatcher_id;
    int     is_handle_in_the_table =  0;
    int     empty_slot_id          = -1;
    int     topic_id_unique        =  1;
    for (int i = 0; i < ADP_DISPATCHER_TABLE_SIZE; ++i) {
        if (dispatcher_table[i].handle == dispatcher_hnd) {
            is_handle_in_the_table = 1;
            dispatcher_id = dispatcher_table[i].dispatcher_id;
        }
        if (dispatcher_table[i].topic_id == 0x00000000) {
            if ( (dispatcher_table[i].handle == 0) || (dispatcher_table[i].handle == dispatcher_hnd) ) {
                if (empty_slot_id == -1) {
                    empty_slot_id = i;
                }
                continue;
            }
        }
        if (dispatcher_table[i].topic_id == topic_id) {
            topic_id_unique = 0;
        }
    }

    // No empty slot is found
    if (empty_slot_id == -1) {
        adp_log_e("Topic:0x%x: No space left in DB", topic_id);
        return ADP_RESULT_NO_SPACE_LEFT;
    }

    // The handle is not found
    if (is_handle_in_the_table == 0) {
        adp_log_e("Handler 0x%x not found in DB", dispatcher_hnd);
        return ADP_RESULT_INVALID_PARAMETER;
    }

    // The topic already registered
    if (topic_id_unique == 0) {
        adp_log_e("Topic:0x%x was already registered", topic_id);
        return ADP_RESULT_INVALID_PARAMETER;
    }

    // Add new record
    dispatcher_table[empty_slot_id].handle        = dispatcher_hnd;
    dispatcher_table[empty_slot_id].dispatcher_id = dispatcher_id;
    dispatcher_table[empty_slot_id].topic_id      = topic_id;
    snprintf(dispatcher_table[empty_slot_id].topic_name, ADP_DISPATCHER_TOPIC_NAME_SIZE, "%s", topic_name);
    adp_log_d("Topic 0x%08x '%s' registered at dispatcher #%d",
            topic_id, topic_name, dispatcher_id);

    return ADP_RESULT_SUCCESS;
}


adp_result_t adp_topic_publish(uint32_t topic_id, const void * data, uint32_t data_length, adp_topic_prio_t prio)
{
    if (topic_id) {
        for (int i = 0; i < ADP_DISPATCHER_TABLE_SIZE; ++i) {
            if (!dispatcher_table[i].handle) {
                continue;
            }
            // Find the handle
            if (dispatcher_table[i].topic_id != topic_id) {
                continue;
            }
            // Allocate space
            adp_generic_msg_t msg;
            if ( (!data) || (!data_length) ) {
                msg.data = NULL;
            } else {
                msg.data = adp_os_malloc(data_length);
                if (!msg.data) {
                    return ADP_RESULT_MALLOC_FAILED;
                }
                memcpy(msg.data, data, data_length);
            }
            msg.length = data_length;
            msg.topic_id = topic_id;
            // Publish to dispatcher
            adp_log_d("Publishing to 0x%08x '%s' size %d", topic_id, dispatcher_table[i].topic_name, data_length);
            if (ADP_RESULT_SUCCESS != adp_os_queue_put(dispatcher_table[i].handle, &msg)) {
                if (msg.data) {
                    adp_os_free(msg.data);
                }
                adp_log_e("Failed to publish to 0x%08x '%s' size %d", topic_id,
                        dispatcher_table[i].topic_name, data_length);
                return ADP_RESULT_NO_SPACE_LEFT;
            }
            adp_os_sleep(0);
            return ADP_RESULT_SUCCESS;
        }
    }
    adp_log_e("Unable to publish to 0x%08x (not registered)", topic_id);
    return ADP_RESULT_INVALID_PARAMETER;
}


adp_result_t adp_topic_subscribe (uint32_t topic_target, adp_topic_cb subscriber_cb, const char * subscriber_name)
{
    if (!subscriber_name) {
        subscriber_name = "";
    }
    for (int i = 0; i < ADP_SUBSCRIBER_TABLE_SIZE; ++i) {
        // Find an empty slot and store the subscription details
        if (!subscriber_table[i].dest_cb) {
            subscriber_table[i].topic_target = topic_target;
            subscriber_table[i].dest_cb      = subscriber_cb;
            snprintf(subscriber_table[i].dest_cb_name, ADP_DISPATCHER_SUBSCRIBER_NAME_SIZE, "%s",  subscriber_name);
            adp_log_d("Subscriber '%s' registered for 0x%08x", subscriber_name, topic_target);
            // Print all topics that corresponds to the mask
            for (int k = 0; k < ADP_DISPATCHER_TABLE_SIZE; ++k) {
                if (!dispatcher_table[k].topic_id) {
                    continue;
                }
                if (dispatcher_table[k].topic_id == topic_target) {
                    adp_log_d("Subscriber '%s' will be notified on topic '%s' 0x%08x",
                            subscriber_name, dispatcher_table[k].topic_name, dispatcher_table[k].topic_id);
                }
            }
            return ADP_RESULT_SUCCESS;
        }
    }
    adp_log_e("Subscriber '%s' NOT registered (no space left) for 0x%08x", subscriber_name, topic_target);
    return ADP_RESULT_NO_SPACE_LEFT;
}


int dispatcher_add(adp_dispatcher_handle_t handle, int id)
{
    int i = 0;
    while ( (i < ADP_DISPATCHER_TABLE_SIZE) && (dispatcher_table[i].handle != NULL) ) {
        i++;
    }
    if (i == ADP_DISPATCHER_TABLE_SIZE) {
        adp_log_e("Unable to create dispatcher: No space left in DB");
        return ADP_RESULT_NO_SPACE_LEFT;
    }
    memset(&dispatcher_table[i], 0x00, sizeof(adp_dispatcher_t));
    dispatcher_table[i].dispatcher_id = id;
    dispatcher_table[i].handle        = handle;
    adp_log_d("New dispatcher #%d is registered", id);

    return ADP_RESULT_SUCCESS;
}


void dispatcher_route_to_dest(adp_os_queue_handle_t queue, const adp_generic_msg_t *msg)
{
    int no_subscriber = 1;
    char *name        = "";
    uint32_t topic_id = msg->topic_id;
    adp_result_t sum_results;
    memset(&sum_results, 0xFF, sizeof(adp_result_t));

    // Get topic_name
    for (int i = 0; i < ADP_DISPATCHER_TABLE_SIZE; ++i) {
        if (dispatcher_table[i].topic_id == topic_id)
            name = dispatcher_table[i].topic_name;
    }
    for (int i = 0; i < ADP_SUBSCRIBER_TABLE_SIZE; ++i) {
        if (!subscriber_table[i].dest_cb) {
            continue;
        }
        uint32_t topic_target = subscriber_table[i].topic_target;
        if (topic_id == topic_target) {
            no_subscriber = 0;
            adp_log_d("Topic 0x%08x '%s' -> subscriber '%s'", topic_id, name, subscriber_table[i].dest_cb_name);
            sum_results &= subscriber_table[i].dest_cb(topic_id, msg->data, msg->length);
        }
    }
    if (ADP_RESULT_SUCCESS != sum_results) {
        adp_log_e("Topic 0x%08x '%s'-> out of interests for all subscribers", topic_id, name);
    }
    if (no_subscriber) {
        adp_log_e("No subscriber found for 0x%08x '%s'", msg->topic_id, name);
    }
}


void dispatcher_task(void* data)
{
    const char* const     name  = adp_os_get_task_name();
    adp_os_queue_handle_t queue = (adp_os_queue_handle_t)data;
    const int             total = adp_queue_msg_total(queue);
    adp_generic_msg_t     msg;

    adp_log_d("Dispatcher %s started, msg capacity %d", name, total);

    // Main dispatcher loop
    while (1) {
        adp_result_t result = adp_queue_receive(queue, &msg, 1000 /* = 1 sec */);
        if (ADP_RESULT_SUCCESS == result) {
            //
            // Route incoming message to its destination
            //
            dispatcher_route_to_dest(queue, &msg);

            // Info about queue
            adp_log_d("Queue usage: %d/%d", adp_queue_msg_waiting(queue), total);

            // The message data is processed, thus free the space
            if (msg.data) {
                adp_os_free(msg.data);
            }
         } else if (ADP_RESULT_TIMEOUT == result) {
             adp_dispatcher_cycle(0, queue); // this function could be used for e.g. stats
         } else {
             adp_log_e("Unable to read from queue");
         }
        adp_dispatcher_cycle(1, queue);
    }

    // Remove compiler warnings
    UNUSED_VAR(name);
    UNUSED_VAR(total);
}


adp_dispatcher_handle_t adp_dispatcher_create(const char* name, uint32_t os_prio, uint32_t max_items)
{
    static int d_cnt = 0;

    if (!d_cnt) {
        memset(dispatcher_table, 0x00, sizeof(dispatcher_table));
        memset(subscriber_table, 0x00, sizeof(subscriber_table));
    }
    // Create queue
    adp_os_queue_handle_t queue = adp_os_queue_create(max_items, sizeof(adp_generic_msg_t));
    if (!queue) {
        ADP_ASSERT(0, "Unable to create a queue");
        return (adp_dispatcher_handle_t)NULL;
    }

    // Start dispatcher with the queue
    int result = adp_os_start_task(name, &dispatcher_task, 512, os_prio, (void*)queue);

    if (ADP_RESULT_SUCCESS == result) {
        if (ADP_RESULT_SUCCESS == dispatcher_add((adp_dispatcher_handle_t)queue, d_cnt)) {
            d_cnt++;
            return (adp_dispatcher_handle_t)queue;
        }
    }

    ADP_ASSERT(0, "Unable to create a dispatcher");
    return (adp_dispatcher_handle_t)NULL;
}


