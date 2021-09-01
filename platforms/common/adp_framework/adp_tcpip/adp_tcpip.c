/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdlib.h>

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "adp_osal.h"
#include "adp_logging.h"
#include "adp_tcpip.h"


#ifdef ADP_TCPIP_MODULE_NO_DEBUG
 #ifdef adp_log_d
  #undef  adp_log_d
 #endif
 #define adp_log_d(...)
#endif

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
    return rand();
}


const char* pcApplicationHostnameHook(void)
{
    return ADP_NET_HOSTNAME;
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
    adp_net_tcpip_status_t adp_net_status;

    /* If the network has just come up...*/
    if( eNetworkEvent == eNetworkUp )
    {
        adp_net_status = ADP_NET_TCPIP_STACK_UP;
        adp_log_d("IP net is up");
        FreeRTOS_GetAddressConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress );
        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
        adp_log_d("IP Address: %s", cBuffer ) ;

        FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
        adp_log_d ( "Subnet Mask: %s", cBuffer ) ;

        FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
        adp_log_d ( "Gateway Address: %s", cBuffer ) ;

        FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
        adp_log_d ( "DNS Server Address: %s", cBuffer ) ;

        event_data[1] = ulIPAddress;
        event_data[2] = ulNetMask;
        event_data[3] = ulGatewayAddress;
        event_data[4] = ulDNSServerAddress;
    } else {
        adp_net_status = ADP_NET_TCPIP_STACK_DOWN;
        event_data[1] = 0;
        event_data[2] = 0;
        event_data[3] = 0;
        event_data[4] = 0;
        adp_log_d("IP net is down" );
    }

    // Publish info
    event_data[0] = adp_net_status;
    adp_topic_publish(ADP_TOPIC_NET_TCPIP_STATUS, &event_data, sizeof(event_data), ADP_TOPIC_PRIORITY_HIGH);
}


adp_result_t adp_tcpip_initialize(adp_dispatcher_handle_t dispatcher)
{
    // Register topics
    adp_topic_register(dispatcher, ADP_TOPIC_NET_TCPIP_STATUS, "NET.Status");

    /*
     ***NOTE*** Tasks that use the network are created in the network event hook
     * when the network is connected and ready for use (see the definition of
     * vApplicationIPNetworkEventHook() below).  The address values passed in here
     * are used if ipconfigUSE_DHCP is set to 0, or if ipconfigUSE_DHCP is set to 1
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

