/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdint.h>

#include "adp_logging.h"
#include "adp_tcpip.h"


int app_net_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_ipnet_status_t status = *(adp_ipnet_status_t*)data;

    adp_log("Network is %s", (status == ADP_IPNET_STACK_DOWN) ? "down" : "up");

    return ADP_RESULT_SUCCESS;
}
