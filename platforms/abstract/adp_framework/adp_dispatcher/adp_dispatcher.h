/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADP_DISPATCHER_H_
#define ADP_DISPATCHER_H_

#include <stdint.h>

#include "adp_osal.h"


typedef enum {
    ADP_TOPIC_PRIORITY_NORMAL  = 0,
    ADP_TOPIC_PRIORITY_HIGH    = 1,
    ADP_TOPIC_PRIORITY_MAX     = ADP_TOPIC_PRIORITY_HIGH,
} adp_topic_prio_t;

typedef void* adp_dispatcher_handle_t;

typedef adp_result_t (adp_topic_cb)(uint32_t topic_id, void* data, uint32_t len);


adp_dispatcher_handle_t adp_dispatcher_create(uint32_t os_prio, uint32_t max_items);

adp_result_t adp_topic_register(adp_dispatcher_handle_t dispatcher_hnd, uint32_t topic_id, const char* topic_name);

adp_result_t adp_topic_publish(uint32_t topic_id, const void* const data, uint32_t data_length, adp_topic_prio_t prio);

adp_result_t adp_topic_subscribe (uint32_t topic_mask, adp_topic_cb subscriber_cb, const char * subscriber_name);

void adp_dispatcher_db_print(adp_dispatcher_handle_t dispatcher_handle);

#endif /* ADP_DISPATCHER_H_ */
