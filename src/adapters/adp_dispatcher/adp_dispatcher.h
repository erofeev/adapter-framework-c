/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADP_DISPATCHER_H_
#define ADP_DISPATCHER_H_

typedef enum {
    ADP_MQTT_MODULE = 0,


    ADP_UNKNOWN     = 0xffffffff,
} adp_module;


void adp_dispatcher_cycle(void);

void adp_dispatcher_loop (void);

void adp_dispatcher_loop1 (void);

#endif /* ADP_DISPATCHER_H_ */
