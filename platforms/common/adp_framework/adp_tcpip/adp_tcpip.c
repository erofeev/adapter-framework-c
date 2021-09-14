/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdlib.h>

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"

#include "adp_osal.h"
#include "adp_logging.h"
#include "adp_tcpip.h"


#ifdef ADP_TCPIP_MODULE_NO_DEBUG
    #undef  adp_log_d
    #undef  adp_log_dd
    #define adp_log_d(...)
    #define adp_log_dd(...)
#endif

typedef struct {
    void               *user_ctx;
    adp_socket_t          socket;
} adp_ipnet_session_t;

adp_ipnet_session_t   s_socket_db[ADP_IPNET_SOCKETS_MAX_NUMBER] = {0};
adp_os_mutex_t        s_socket_list_mutex = NULL;


/* The default IP and MAC address.  The address configuration
 * defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
 * 1 but a DHCP server could not be contacted.  See the online documentation for
 * more information. */
static const uint8_t adp_net_self_ip_addr[4] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t adp_net_ip_mask[4]      = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t adp_net_gw_ip_addr[4]   = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t adp_net_dns_addr[4]     = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* Default MAC address configuration.  The demo creates a virtual network
 * connection that uses this MAC address by accessing the raw Ethernet data
 * to and from a real network connection on the host PC.  See the
 * configNETWORK_INTERFACE_TO_USE definition for information on how to configure
 * the real network connection to use. */
const uint8_t adp_net_mac_addr[6] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

/*
 * Callback that provides the inputs necessary to generate a randomized TCP
 * Initial Sequence Number per RFC 6528.
 */
uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
                                                    uint16_t usSourcePort,
                                                    uint32_t ulDestinationAddress,
                                                    uint16_t usDestinationPort )
{
    UNUSED_VAR(ulSourceAddress);
    UNUSED_VAR(usSourcePort);
    UNUSED_VAR(ulDestinationAddress);
    UNUSED_VAR(usDestinationPort);
    return adp_os_rand();
}


const char* pcApplicationHostnameHook(void)
{
    return ADP_IPNET_HOSTNAME;
}


BaseType_t xApplicationDNSQueryHook(const char *pcName)
{
    BaseType_t xReturn;

    // Determine if a name lookup is for this node.
    if (strcasecmp(pcName, pcApplicationHostnameHook()) == 0) {
        xReturn = pdPASS;
    } else {
        xReturn = pdFAIL;
    }

    return xReturn;
}


void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
    char cBuffer[ 16 ];
    uint32_t event_data[5] = {0};
    adp_ipnet_status_t adp_net_status;

    /* If the network has just come up...*/
    if( eNetworkEvent == eNetworkUp )
    {
        adp_net_status = ADP_IPNET_STACK_UP;
        adp_log_d("IP net is up");
        FreeRTOS_GetAddressConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress );
        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer);
        adp_log_d("IP Address: %s", cBuffer) ;

        FreeRTOS_inet_ntoa( ulNetMask, cBuffer);
        adp_log_d("Subnet Mask: %s", cBuffer);

        FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer);
        adp_log_d("Gateway Address: %s", cBuffer);

        FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer);
        adp_log_d("DNS Server Address: %s", cBuffer);

        event_data[1] = ulIPAddress;
        event_data[2] = ulNetMask;
        event_data[3] = ulGatewayAddress;
        event_data[4] = ulDNSServerAddress;
    } else {
        adp_net_status = ADP_IPNET_STACK_DOWN;
        event_data[1] = 0;
        event_data[2] = 0;
        event_data[3] = 0;
        event_data[4] = 0;
        adp_log_d("IP net is down" );
    }

    // Publish info
    event_data[0] = adp_net_status;
    adp_topic_publish(ADP_TOPIC_IPNET_STATUS, &event_data, sizeof(event_data), ADP_TOPIC_PRIORITY_HIGH);
}

static
adp_result_t ipnet_add_socket(void *user_ctx, adp_socket_t socket)
{
    adp_os_mutex_take(s_socket_list_mutex);

    for (int i = 0; i < ADP_IPNET_SOCKETS_MAX_NUMBER; i++) {
        if (!s_socket_db[i].socket)
            continue;
        if ( (s_socket_db[i].user_ctx == user_ctx) ||
             (s_socket_db[i].socket   == socket  ) ) {
            adp_os_mutex_give(s_socket_list_mutex);
            adp_log_e("Socket 0x%x already in the list for userCtx 0x%x", socket, user_ctx);
            return ADP_RESULT_INVALID_PARAMETER;
        }
    }
    for (int i = 0; i < ADP_IPNET_SOCKETS_MAX_NUMBER; i++) {
        if (s_socket_db[i].socket == NULL) {
            s_socket_db[i].socket   = socket;
            s_socket_db[i].user_ctx = user_ctx;
            adp_os_mutex_give(s_socket_list_mutex);
            adp_log_dd("New socket 0x%x added to the list for userCxt 0x%x", socket, user_ctx);
            return ADP_RESULT_SUCCESS;
        }
    }
    adp_os_mutex_give(s_socket_list_mutex);
    adp_log_e("Cannot add new socket 0x%x to the list", socket);
    return ADP_RESULT_NO_SPACE_LEFT;
}

static
adp_socket_t* ipnet_find_socket_by_user_ctx(void *user_ctx)
{
    adp_os_mutex_take(s_socket_list_mutex);
    for (int i = 0; i < ADP_IPNET_SOCKETS_MAX_NUMBER; i++) {
        if (s_socket_db[i].user_ctx == user_ctx) {
            adp_os_mutex_give(s_socket_list_mutex);
            return s_socket_db[i].socket;
        }
    }
    adp_os_mutex_give(s_socket_list_mutex);
    adp_log_e("Socket not in the list for userCtx 0x%x", user_ctx);
    return NULL;
}

static
adp_socket_t* ipnet_is_socket_in_list(void *socket)
{
    adp_os_mutex_take(s_socket_list_mutex);
    for (int i = 0; i < ADP_IPNET_SOCKETS_MAX_NUMBER; i++) {
        if (s_socket_db[i].socket == socket) {
            adp_os_mutex_give(s_socket_list_mutex);
            return s_socket_db[i].socket;
        }
    }
    adp_os_mutex_give(s_socket_list_mutex);
    return NULL;
}

static
void ipnet_del_socket(adp_socket_t *socket)
{
    adp_os_mutex_take(s_socket_list_mutex);
    for (int i = 0; i < ADP_IPNET_SOCKETS_MAX_NUMBER; i++) {
        if (s_socket_db[i].socket == socket) {
            s_socket_db[i].socket   = NULL;
            s_socket_db[i].user_ctx = NULL;
            adp_os_mutex_give(s_socket_list_mutex);
            adp_log_dd("Socket 0x%x removed from the list", socket);
            return;
        }
    }
    adp_os_mutex_give(s_socket_list_mutex);
    adp_log_e("Socket 0x%x not in the list", socket);
    return;
}

adp_socket_t adp_ipnet_socket_alloc(adp_socket_option_t option)
{
    Socket_t xSocket = FREERTOS_INVALID_SOCKET;

    if (option == ADP_SOCKET_TCP) {
        /* Create a TCP socket. */
        xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    } else {
        // Not supported
        adp_log_e("Unsupported socket option");
    }

    if( xSocket == FREERTOS_INVALID_SOCKET )
    {
        return NULL;
    }
    return (adp_socket_t)xSocket;
}


void adp_ipnet_socket_free(adp_socket_t socket)
{
    if (socket) {
        // Disable monitoring
        FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_WAKEUP_CALLBACK, NULL, 0);
        FreeRTOS_shutdown(socket, 0 /* not used */);
        int i = FreeRTOS_closesocket(socket);
        if (1 != i) {
            adp_log_e("Unable to close socket 0x%x, error %d", socket, i);
        }

    } else {
        adp_log_e("Bug: socket is null");
    }
}


uint32_t adp_ipnet_socket_send(adp_socket_t socket, const void *buffer, int bytesToSend)
{
    int32_t socketStatus;

    socketStatus = FreeRTOS_send(socket, buffer, bytesToSend, 0);

    adp_log_dd("Note: socket 0x%x TX space = %d (socket status %d)", socket, FreeRTOS_tx_space(socket), (int)socketStatus);

    if (socketStatus == -pdFREERTOS_ERRNO_ENOSPC) {
        /* The TCP buffers could not accept any more bytes so zero bytes were sent.
         * This is not necessarily an error that should cause a disconnect unless
         * it persists so return 0 bytes received rather than an error. */
        socketStatus = 0;
    }
    return socketStatus;
}


uint32_t adp_ipnet_socket_recv(adp_socket_t socket, void *buffer, int bytesToRecv)
{
    int32_t socketStatus = 1;

    /* The TCP socket may have a receive block time.  If bytesToRecv is greater
     * than 1, then a frame is likely already part way through reception and
     * blocking to wait for the desired number of bytes to be available is the
     * most efficient thing to do.  If bytesToRecv is 1, then this may be a
     * speculative call to read to find the start of a new frame, in which case
     * blocking is not desirable as it could block an entire protocol agent
     * task for the duration of the read block time and therefore negatively
     * impact performance.  So if bytesToRecv is 1, then don't call recv unless
     * it is known that bytes are already available. */
    if (bytesToRecv == 1) {
        socketStatus = (int32_t) FreeRTOS_recvcount(socket);
    }

    if (socketStatus > 0) {
        socketStatus = FreeRTOS_recv(socket, buffer, bytesToRecv, 0);
    }
    uint32_t size = FreeRTOS_rx_size(socket);
    if (size) {
        adp_log_dd("Note: socket 0x%x RX remains %d bytes", socket, FreeRTOS_rx_size(socket));
    }
    return socketStatus;
}

static
void ipnet_client_socket_wakeup_cb(adp_socket_t socket)
{
    if (!ipnet_is_socket_in_list(socket)) {
        adp_log_dd("Socket wakeup received for socket that is closing");
    }
    if (pdTRUE != FreeRTOS_issocketconnected(socket)) {
        adp_log_dd("Disconnect on socket 0x%x", socket);
        adp_topic_publish(ADP_TOPIC_IPNET_SOCKET_DISCONNECTED, &socket, sizeof(adp_socket_t), ADP_TOPIC_PRIORITY_HIGH);
    } else {
        adp_log_dd("Detected rx/tx activity on socket 0x%x", socket);
        adp_topic_publish(ADP_TOPIC_IPNET_SOCKET_RXTX_ACTIVITY, &socket, sizeof(adp_socket_t), ADP_TOPIC_PRIORITY_HIGH);
    }
}


adp_result_t ipnet_do_tcp_connect(adp_ipnet_cmd_status_t *result, adp_ipnet_cmd_t* cmd_data)
{
    uint32_t ipaddr;
    char addr_str[16];
    char *hostname = "";
    Socket_t xSocket;
    adp_ipnet_cmd_connect_t *connect = (adp_ipnet_cmd_connect_t*)&cmd_data->connect;
    struct freertos_sockaddr xRemoteAddress;

    result->socket = NULL;

    if (!connect) {
        result->status  = ADP_RESULT_FAILED;
        adp_log_e("Connect is NULL, userCtx 0x%x", cmd_data->user_ctx);
        return ADP_RESULT_FAILED;
    }

    // Create socket
    xSocket = adp_ipnet_socket_alloc(ADP_SOCKET_TCP);
    if (!xSocket) {
        result->status  = ADP_RESULT_FAILED;
        adp_log_e("Socket allocation failed, userCtx 0x%x", cmd_data->user_ctx);
        return ADP_RESULT_FAILED;
    }

    if (connect->hostname) {
        hostname = connect->hostname;
        adp_log_d("DNS search for %s", hostname);
        ipaddr = FreeRTOS_gethostbyname(hostname);
    } else {
        ipaddr = FreeRTOS_inet_addr_quick(connect->ip_octet1, connect->ip_octet2, connect->ip_octet3, connect->ip_octet4);
    }
    FreeRTOS_inet_ntoa(ipaddr, addr_str);
    adp_log_d("Trying to connect to %s:%d [%s]", addr_str, connect->port, hostname);
    xRemoteAddress.sin_addr = ipaddr;
    xRemoteAddress.sin_port = FreeRTOS_htons(connect->port);

    BaseType_t status = FreeRTOS_connect(xSocket, &xRemoteAddress, sizeof(xRemoteAddress));
    if (status != pdFREERTOS_ERRNO_NONE) {
        // Failed to connect to the server
        result->status  = ADP_RESULT_FAILED;
        result->subcode = status;
        adp_log_d("Failed to connect, userCtx 0x%x", cmd_data->user_ctx);
        adp_ipnet_socket_free(xSocket);
        return ADP_RESULT_FAILED;
    }

    // Start monitoring activity on the socket
    status = FreeRTOS_setsockopt(xSocket,
                                  0, /* Level - Unused. */
                                  FREERTOS_SO_WAKEUP_CALLBACK,
                                  (void*)ipnet_client_socket_wakeup_cb,
                                  sizeof(&(ipnet_client_socket_wakeup_cb)));
    if (status != pdFREERTOS_ERRNO_NONE) {
        // Failed to connect to the server
        result->status  = ADP_RESULT_FAILED;
        result->subcode = status;
        adp_ipnet_socket_free(xSocket);
        adp_log_e("Failed to set sock options, userCtx 0x%x", cmd_data->user_ctx);
        return ADP_RESULT_FAILED;
    } else {
        if (ADP_RESULT_SUCCESS != ipnet_add_socket(cmd_data->user_ctx, xSocket)) {
            result->status  = ADP_RESULT_FAILED;
            adp_log_e("Failed to add socket, userCtx 0x%x", cmd_data->user_ctx);
            adp_ipnet_socket_free(xSocket);
            return ADP_RESULT_FAILED;
        }
    }
    adp_log_dd("Connected socket 0x%x, userCtx 0x%x", xSocket, cmd_data->user_ctx);
    result->socket = xSocket;
    result->status = ADP_RESULT_SUCCESS;

    return ADP_RESULT_SUCCESS;
}


adp_result_t ipnet_do_tcp_shutdown(adp_ipnet_cmd_status_t *result, adp_ipnet_cmd_t* cmd_data)
{
    adp_socket_t socket = ipnet_find_socket_by_user_ctx(cmd_data->user_ctx);
    result->socket = socket;
    if (socket) {
        ipnet_del_socket(socket);
        adp_ipnet_socket_free(socket);
        result->status = ADP_RESULT_SUCCESS;
    } else {
        result->status = ADP_RESULT_INVALID_PARAMETER;
    }
    return result->status;
}


int ipnet_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_ipnet_cmd_t *cmd = (adp_ipnet_cmd_t*)data;
    adp_ipnet_cmd_status_t result;

    memset(&result, 0x00, sizeof(adp_ipnet_cmd_status_t));

    result.user_ctx = cmd->user_ctx;
    result.command  = cmd->command;
    result.subcode  = 0xADFF;

    switch (cmd->command) {
    case ADP_IPNET_DO_TCP_CONNECT:
        {
            adp_log_d("IPNET - DO_TCP_CONNECT userCtx 0x%x", cmd->user_ctx);
            ipnet_do_tcp_connect(&result, (adp_ipnet_cmd_t*)data);
        }
        break;
    case ADP_IPNET_DO_TCP_SHUTDOWN:
        {
            adp_log_d("IPNET - DO_TCP_SHUTDOWN userCtx 0x%x", cmd->user_ctx);
            ipnet_do_tcp_shutdown(&result, (adp_ipnet_cmd_t*)data);
        }
        break;
    default:
        adp_log_e("Unknown IPNET cmd #%d", cmd->command);
        result.status = ADP_RESULT_INVALID_PARAMETER;
        break;
    }

    // Notify users on status change
    adp_topic_publish(ADP_TOPIC_IPNET_CMD_STATUS, &result, sizeof(adp_ipnet_cmd_status_t), ADP_TOPIC_PRIORITY_HIGH);

    return ADP_RESULT_SUCCESS;
}


adp_result_t adp_ipnet_initialize(adp_dispatcher_handle_t dispatcher)
{
    memset(s_socket_db, 0x00, sizeof(s_socket_db));
    s_socket_list_mutex = adp_os_mutex_create();

    // Published topics
    adp_topic_register(dispatcher, ADP_TOPIC_IPNET_STATUS,               "IPNET.Status");
    adp_topic_register(dispatcher, ADP_TOPIC_IPNET_EXECUTE_CMD,          "IPNET.ExecuteCmd");
    adp_topic_register(dispatcher, ADP_TOPIC_IPNET_CMD_STATUS,           "IPNET.CmdStatus");
    adp_topic_register(dispatcher, ADP_TOPIC_IPNET_SOCKET_RXTX_ACTIVITY, "IPNET.SocketIOActivity");
    adp_topic_register(dispatcher, ADP_TOPIC_IPNET_SOCKET_DISCONNECTED,  "IPNET.SocketIODisconnect");

    // Subscribed for topics
    adp_topic_subscribe(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_cmd_handler,  "ADP.IPNET.SVC.Executor");

    /*
     * The address values passed in here are used if ipconfigUSE_DHCP
     * is set to 0, or if ipconfigUSE_DHCP is set to 1
     * but a DHCP server cannot be  contacted. */
    BaseType_t result = FreeRTOS_IPInit(adp_net_self_ip_addr, adp_net_ip_mask,
            adp_net_gw_ip_addr, adp_net_dns_addr, adp_net_mac_addr);

    if (result == pdTRUE) {
        adp_log_d("TCP/IP stack initialized");
        return ADP_RESULT_SUCCESS;
    }
    adp_log_e("Failed to initialize TCP/IP stack");
    return ADP_RESULT_FAILED;
}

