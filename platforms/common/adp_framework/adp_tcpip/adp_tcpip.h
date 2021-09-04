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
    ADP_SOCKET_TCP                = 0,
} adp_socket_option_t;

typedef enum {
    ADP_IPNET_STACK_DOWN          = 0,
    ADP_IPNET_STACK_UP            = 1,
    ADP_IPNET_STACK_NA            = 0xFF,
} adp_ipnet_status_t;

typedef enum {
    ADP_IPNET_DO_TCP_CONNECT      = 0x00000001,
} adp_ipnet_command_code_t;

typedef void* adp_socket_t;

// The content of ADP_TOPIC_IPNET_CMD_STATUS
typedef struct {
    adp_ipnet_command_code_t    command;
    int32_t                      status;
    int32_t                     subcode;
    adp_socket_t                 socket;
} adp_ipnet_cmd_status_t;

// The content of ADP_TOPIC_IPNET_EXECUTE_CMD - ADP_IPNET_DO_TCP_CONNECT
typedef struct {
    uint16_t                       port;
    char                      *hostname;
    uint8_t                   ip_octet1;
    uint8_t                   ip_octet2;
    uint8_t                   ip_octet3;
    uint8_t                   ip_octet4;
} adp_ipnet_cmd_connect_t;

// The content of ADP_TOPIC_IPNET_EXECUTE_CMD
typedef struct {
    adp_ipnet_command_code_t     command;
    adp_socket_t                  socket;
    union {
        adp_ipnet_cmd_connect_t  connect;
    };
} adp_ipnet_cmd_t;


adp_socket_t adp_ipnet_socket_alloc(adp_socket_option_t option);

void adp_ipnet_socket_free(adp_socket_t socket);

uint32_t adp_ipnet_socket_send(adp_socket_t socket, void *buffer, int bytesToSend);

uint32_t adp_ipnet_socket_recv(adp_socket_t socket, void *buffer, int bytesToRecv);

adp_result_t adp_ipnet_initialize(adp_dispatcher_handle_t dispatcher);


#endif /* ADAPTERS_ADP_TCPIP_H_ */
