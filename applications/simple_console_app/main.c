/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_console.h"
#include "adp_logging.h"

#include "app_config.h"
#include "app_console.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"

void test_subpub(void* params)
{
    UNUSED_VAR(params);
    int interval = 15; // 15 seconds

    while(1) {
        adp_os_sleep(1000 * interval);
        adp_log("uptime is %d seconds", adp_os_uptime());

        // Print the whole DB
        char  *data = "db\n";
        adp_topic_publish(ADP_TOPIC_SYSTEM_CLI_INPUT_STREAM, data, strlen(data) + 1, ADP_TOPIC_PRIORITY_HIGH);
    }
}


int main(void) {
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("Info-print", &test_subpub, 512, 6, NULL);


    // Example of first dispatcher creation with task prio 5 and items number 25
    adp_dispatcher_handle_t dispatcher_1 = adp_dispatcher_create(5, 25);
    adp_dispatcher_handle_t dispatcher_2 = adp_dispatcher_create(4, 35);
    adp_dispatcher_handle_t dispatcher_3 = adp_dispatcher_create(4, 45);

    // Example of adding few topics
    uint32_t topic_id   = 0x00000010;
    adp_topic_register(dispatcher_1, topic_id, "Temperature");
    adp_topic_register(dispatcher_1, topic_id | 0x1, "getCurrent");
    adp_topic_register(dispatcher_1, topic_id | 0x2, "getPrev");
    adp_topic_register(dispatcher_1, topic_id | 0x3, "getAverage");
    adp_topic_register(dispatcher_2, 0x00000100, "Sensors");
    adp_topic_register(dispatcher_3, 0x00000200, "Connectivity");
    adp_topic_register(dispatcher_1, 0x10000000, "");

    // Run console
    adp_dispatcher_handle_t system_dispatcher = adp_dispatcher_create(0, 25);
    adp_os_start_task("Console", &adp_console_task, 128, 0, system_dispatcher);

    // Add app handler for receiving CLI commands requested for the execution
    adp_topic_subscribe(ADP_TOPIC_SYSTEM_CLI_EXECUTE_CMD, &app_cmd_handler, "App CMD handler");


    main_tcp_echo_client_tasks();


    adp_os_start();

    return EXIT_SUCCESS;
}






#include "task.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"









/* The default IP and MAC address used by the demo.  The address configuration
 * defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
 * 1 but a DHCP server could not be contacted.  See the online documentation for
 * more information. */
static const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* Set the following constant to pdTRUE to log using the method indicated by the
 * name of the constant, or pdFALSE to not log using the method indicated by the
 * name of the constant.  Options include to standard out (xLogToStdout), to a disk
 * file (xLogToFile), and to a UDP port (xLogToUDP).  If xLogToUDP is set to pdTRUE
 * then UDP messages are sent to the IP address configured as the echo server
 * address (see the configECHO_SERVER_ADDR0 definitions in FreeRTOSConfig.h) and
 * the port number set by configPRINT_PORT in FreeRTOSConfig.h. */
const BaseType_t xLogToStdout = pdTRUE, xLogToFile = pdFALSE, xLogToUDP = pdFALSE;

/* Default MAC address configuration.  The demo creates a virtual network
 * connection that uses this MAC address by accessing the raw Ethernet data
 * to and from a real network connection on the host PC.  See the
 * configNETWORK_INTERFACE_TO_USE definition for information on how to configure
 * the real network connection to use. */
const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

/* Use by the pseudo random number generator. */
static UBaseType_t ulNextRand;

void main_tcp_echo_client_tasks( void )
{
    const uint32_t ulLongTime_ms = pdMS_TO_TICKS( 1000UL );

    /*
     * Instructions for using this project are provided on:
     * http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/examples_FreeRTOS_simulator.html
     */

    /* Miscellaneous initialisation including preparing the logging and seeding
     * the random number generator. */
    prvMiscInitialisation();

    /* Initialise the network interface.
     *
     ***NOTE*** Tasks that use the network are created in the network event hook
     * when the network is connected and ready for use (see the definition of
     * vApplicationIPNetworkEventHook() below).  The address values passed in here
     * are used if ipconfigUSE_DHCP is set to 0, or if ipconfigUSE_DHCP is set to 1
     * but a DHCP server cannot be  contacted. */
    adp_log( ( "FreeRTOS_IPInit\n" ) );
    FreeRTOS_IPInit( ucIPAddress,
                     ucNetMask,
                     ucGatewayAddress,
                     ucDNSServerAddress,
                     ucMACAddress );


}
/*-----------------------------------------------------------*/

/* Called by FreeRTOS+TCP when the network connects or disconnects.  Disconnect
 * events are only received if implemented in the MAC driver. */
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
    char cBuffer[ 16 ];
    static BaseType_t xTasksAlreadyCreated = pdFALSE;

    /* If the network has just come up...*/
    if( eNetworkEvent == eNetworkUp )
    {
        /* Create the tasks that use the IP stack if they have not already been
         * created. */
        if( xTasksAlreadyCreated == pdFALSE )
        {
            /* See the comments above the definitions of these pre-processor
             * macros at the top of this file for a description of the individual
             * demo tasks. */
adp_log("+++++++++++++++++++++++++++++++++++++++++++++++ NETWORK IS UP +++++++++++++++++++++++++");
            #if ( mainCREATE_TCP_ECHO_TASKS_SINGLE == 1 )
                {
                    vStartTCPEchoClientTasks_SingleTasks( 1024, 3 );
                }
            #endif /* mainCREATE_TCP_ECHO_TASKS_SINGLE */

            xTasksAlreadyCreated = pdTRUE;
        }

        /* Print out the network configuration, which may have come from a DHCP
         * server. */
        FreeRTOS_GetAddressConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress );
        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
        adp_log ( "\r\n\r\nIP Address: %s\r\n", cBuffer ) ;

        FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
        adp_log ( "Subnet Mask: %s\r\n", cBuffer ) ;

        FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
        adp_log ( "Gateway Address: %s\r\n", cBuffer ) ;

        FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
        adp_log ( "DNS Server Address: %s\r\n\r\n\r\n", cBuffer ) ;
    }
    else
    {
        FreeRTOS_printf( "Application idle hook network down\n" );
    }
}
/*-----------------------------------------------------------*/

UBaseType_t uxRand( void )
{
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

    /* Utility function to generate a pseudo random number. */

    ulNextRand = ( ulMultiplier * ulNextRand ) + ulIncrement;
    return( ( int ) ( ulNextRand >> 16UL ) & 0x7fffUL );
}
/*-----------------------------------------------------------*/

static void prvSRand( UBaseType_t ulSeed )
{
    /* Utility function to seed the pseudo random number generator. */
    ulNextRand = ulSeed;
}
/*-----------------------------------------------------------*/

 void prvMiscInitialisation( void )
{
    time_t xTimeNow;

    /* Seed the random number generator. */
    time( &xTimeNow );
    adp_log ( "Seed for randomiser: %lu\n", xTimeNow ) ;
    prvSRand( ( uint32_t ) xTimeNow );
    adp_log ( "Random numbers: %08X %08X %08X %08X\n",
                             ipconfigRAND32(),
                             ipconfigRAND32(),
                             ipconfigRAND32(),
                             ipconfigRAND32() ) ;
}
/*-----------------------------------------------------------*/
#define  mainDEVICE_NICK_NAME "FREERTOS_NICK"

 #if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) || ( ipconfigDHCP_REGISTER_HOSTNAME == 1 )

    const char * pcApplicationHostnameHook( void )
    {
        /* Assign the name "FreeRTOS" to this network node.  This function will
         * be called during the DHCP: the machine will be registered with an IP
         * address plus this name. */
        return "FREERTOS";
    }

#endif
/*-----------------------------------------------------------*/

#if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 )

    BaseType_t xApplicationDNSQueryHook( const char * pcName )
    {
        BaseType_t xReturn;

        /* Determine if a name lookup is for this node.  Two names are given
         * to this node: that returned by pcApplicationHostnameHook() and that set
         * by mainDEVICE_NICK_NAME. */
        if( strcasecmp( pcName, pcApplicationHostnameHook() ) == 0 )
        {
            xReturn = pdPASS;
        }
        else if( strcasecmp( pcName, mainDEVICE_NICK_NAME ) == 0 )
        {
            xReturn = pdPASS;
        }
        else
        {
            xReturn = pdFAIL;
        }

        return xReturn;
    }

#endif /* if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) */

/*
 * Callback that provides the inputs necessary to generate a randomized TCP
 * Initial Sequence Number per RFC 6528.  THIS IS ONLY A DUMMY IMPLEMENTATION
 * THAT RETURNS A PSEUDO RANDOM NUMBER SO IS NOT INTENDED FOR USE IN PRODUCTION
 * SYSTEMS.
 */
extern uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
                                                    uint16_t usSourcePort,
                                                    uint32_t ulDestinationAddress,
                                                    uint16_t usDestinationPort )
{
    ( void ) ulSourceAddress;
    ( void ) usSourcePort;
    ( void ) ulDestinationAddress;
    ( void ) usDestinationPort;

    return uxRand();
}

/*
 * Supply a random number to FreeRTOS+TCP stack.
 * THIS IS ONLY A DUMMY IMPLEMENTATION THAT RETURNS A PSEUDO RANDOM NUMBER
 * SO IS NOT INTENDED FOR USE IN PRODUCTION SYSTEMS.
 */
BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
    *( pulNumber ) = uxRand();
    return pdTRUE;
}



