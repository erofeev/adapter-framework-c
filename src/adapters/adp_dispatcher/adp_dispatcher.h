/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADP_DISPATCHER_H_
#define ADP_DISPATCHER_H_

typedef enum {
    ADP_TOPIC_CMD_PRIORITY_HIGH    = 1,
    ADP_TOPIC_CMD_PRIORITY_NORMAL  = 0,
} adp_topic_cmd_prio_t;

typedef void* adp_dispatcher_handle_t;

typedef void (adp_topic_cb)(uint16_t cmd, void* data, uint32_t len, uint32_t flags);


adp_dispatcher_handle_t adp_dispatcher_create(uint32_t os_prio, uint32_t max_items);

int adp_topic_register(adp_dispatcher_handle_t dispatcher_hnd, uint16_t topic_id, const char* topic_name, uint16_t cmd_id, const char* cmd_name, adp_topic_cmd_prio_t prio);

#endif /* ADP_DISPATCHER_H_ */
