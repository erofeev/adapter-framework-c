/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_TCPIP_H_
#define ADAPTERS_ADP_TCPIP_H_

#include "adp_osal.h"
#include "adp_dispatcher.h"


typedef enum {
    ADP_NET_TCPIP_STACK_DOWN       = 0,
    ADP_NET_TCPIP_STACK_UP         = 1,
} adp_net_tcpip_status_t;


adp_result_t adp_tcpip_initialize(adp_dispatcher_handle_t dispatcher);


#endif /* ADAPTERS_ADP_TCPIP_H_ */
