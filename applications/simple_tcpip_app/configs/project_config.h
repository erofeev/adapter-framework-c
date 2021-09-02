/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_


// ADP Dispatcher settings
#define ADP_DISPATCHER_MODULE_NO_DEBUG
#define ADP_DISPATCHER_TABLE_SIZE                    10
#define ADP_DISPATCHER_TOPIC_NAME_SIZE               32  /* Should be greater than 0 */
#define ADP_DISPATCHER_SUBSCIBER_NAME_SIZE           32  /* Should be greater than 0 */
#define ADP_SUBSCRIBER_TABLE_SIZE                     5

// ADP Console settings
#define ADP_CONSOLE_MODULE_NO_DEBUG
#define ADP_CONSOLE_MAX_CMD_SIZE                     40

// ADP Network settings
#undef  ADP_TCPIP_MODULE_NO_DEBUG
#define ADP_NET_HOSTNAME                             "AdpHostName"

// Topics DB
enum app_topics_enum {
    ADP_TOPIC_SYSTEM_RESERVED                       = 0x00000000,
    ADP_TOPIC_CLI_INPUT_STREAM                      = 0x00000001,
    ADP_TOPIC_CLI_EXECUTE_CMD                       = 0x00000002,
    ADP_TOPIC_IPNET_IPSTATUS                        = 0x00000010,
    ADP_TOPIC_IPNET_EXECUTE_CMD                     = 0x00000011,
};


#endif /* APP_CONFIG_H_ */
