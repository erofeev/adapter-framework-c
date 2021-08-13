/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADP_DISPATCHER_H_
#define ADP_DISPATCHER_H_

#define ADP_DISPATCHER_MAX_INSTANCES   20


typedef void (adp_topic_cb)(uint16_t cmd, void* data, uint32_t len, uint32_t flags);


void adp_dispatcher_cycle(void);

void adp_dispatcher(void* data);

int adp_dispatcher_create(uint32_t os_prio, uint32_t max_items);


#endif /* ADP_DISPATCHER_H_ */
