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
    ADP_IPNET_STACK_DOWN          = 0,
    ADP_IPNET_STACK_UP            = 1,
    ADP_IPNET_STACK_NA            = 0xFF,
} adp_ipnet_status_t;

typedef enum {
    ADP_IPNET_DO_TCP_CONNECT      = 0x00000001,
} adp_ipnet_command_code_t;

// The content of ADP_TOPIC_IPNET_EXECUTE_CMD - ADP_IPNET_DO_TCP_CONNECT
typedef struct {
    void                    *session_id;
    uint16_t              clean_session;
    uint16_t         keep_alive_seconds;
    const char               *client_id;
    const char                *username;
    const char                *password;
} adp_ipnet_cmd_connect_t;

// The content of ADP_TOPIC_IPNET_EXECUTE_CMD
typedef struct {
    adp_ipnet_command_code_t     command;
    union {
        adp_ipnet_cmd_connect_t  connect;
    };
} adp_ipnet_cmd_t;


adp_result_t adp_ipnet_initialize(adp_dispatcher_handle_t dispatcher);


#endif /* ADAPTERS_ADP_TCPIP_H_ */
