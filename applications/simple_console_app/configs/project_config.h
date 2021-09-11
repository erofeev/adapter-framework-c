/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

// APP OS settings
#define ADP_OS_MODULE_NO_DEBUG
#define ADP_OS_DEFAULT_QUEUE_PUT_TIMEOUT_MS         100
#define ADP_MEMORY_ALLOC_FREE_TRACE_ENABLED
#define ADP_MEMORY_ALLOC_FREE_TRACE_DB_SIZE          80

// APP Dispatcher settings
#define ADP_DISPATCHER_MODULE_NO_DEBUG
#define ADP_DISPATCHER_TABLE_SIZE                    15
#define ADP_DISPATCHER_TOPIC_NAME_SIZE               32  /* Should be greater than 0 */
#define ADP_DISPATCHER_SUBSCRIBER_NAME_SIZE          32  /* Should be greater than 0 */
#define ADP_SUBSCRIBER_TABLE_SIZE                    15

// APP Console settings
#define ADP_CONSOLE_MODULE_NO_DEBUG
#define ADP_CONSOLE_MAX_CMD_SIZE                     40
#define ADP_CONSOLE_ECHO_ENABLED                      0

// APP Network settings
#undef ADP_TCPIP_MODULE_NO_DEBUG
#define ADP_IPNET_HOSTNAME                           "AdpHostName"
#define ADP_IPNET_SOCKETS_MAX_NUMBER                 10

// APP MQTT settings
#undef ADP_MQTT_MODULE_NO_DEBUG
#define ADP_MQTT_SESSIONS_MAX_NUMBER                 10
#define ADP_MQTT_PROCESS_LOOP_TIMEOUT_MS            300

// Topics DB
enum app_topics_enum {
    ADP_TOPIC_SYSTEM_RESERVED_NOT_USED              = 0x00000000,
    // Command Line Interface
    ADP_TOPIC_CLI_INPUT_STREAM                      = 0x00000001,
    ADP_TOPIC_CLI_EXECUTE_CMD                       = 0x00000002,
};


#define ADP_LOG_USE_COLOR                             1


#endif /* APP_CONFIG_H_ */
