/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

// APP OS settings
#define ADP_OS_MODULE_NO_DEBUG
#define ADP_HEAP_MEMORY_SIZE                  (128*1024)
#define ADP_OS_DEFAULT_QUEUE_PUT_TIMEOUT_MS         100
#define ADP_MEMORY_ALLOC_FREE_TRACE_ENABLED
#define ADP_MEMORY_ALLOC_FREE_TRACE_DB_SIZE         120

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
#define ADP_IPNET_SOCKETS_MAX_NUMBER                 25

// APP MQTT settings
#undef ADP_MQTT_MODULE_NO_DEBUG
#undef ADP_MQTT_AGENT_MODULE_NO_DEBUG
#define ADP_MQTT_SESSIONS_MAX_NUMBER                 25
#define ADP_MQTT_PROCESS_LOOP_TIMEOUT_MS            200

// Topics DB
enum app_topics_enum {
    ADP_TOPIC_SYSTEM_RESERVED_NOT_USED              = 0x00000000,
    // Command Line Interface
    ADP_TOPIC_CLI_INPUT_STREAM                      = 0x00000001,
    ADP_TOPIC_CLI_EXECUTE_CMD                       = 0x00000002,
    // Networking TCP-UDP/IP
    ADP_TOPIC_IPNET_STATUS                          = 0x00000010,
    ADP_TOPIC_IPNET_EXECUTE_CMD                     = 0x00000011,
    ADP_TOPIC_IPNET_CMD_STATUS                      = 0x00000012,
    ADP_TOPIC_IPNET_SOCKET_RXTX_ACTIVITY            = 0x00000013,
    ADP_TOPIC_IPNET_SOCKET_DISCONNECTED             = 0x00000014,
    // MQTT connectivity
    ADP_TOPIC_MQTT_INCOMING_TOPIC                   = 0x00000021,
    ADP_TOPIC_MQTT_EXECUTE_CMD                      = 0x00000022,
    ADP_TOPIC_MQTT_CMD_STATUS                       = 0x00000023,
};


#define ADP_LOG_USE_COLOR                             1


#endif /* APP_CONFIG_H_ */
