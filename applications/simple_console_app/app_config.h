/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_


// ADP Dispatcher settings
#define ADP_DISPATCHER_MODULE_NO_DEBUG
#define ADP_DISPATCHER_TABLE_SIZE              10
#define ADP_SUBSCRIBER_TABLE_SIZE              5


// Topics DB
enum {
    ADP_TOPIC_SYSTEM                   = 0x80000000,
    ADP_TOPIC_SYSTEM_CLI_INPUT_STREAM  = (ADP_TOPIC_SYSTEM | 0x0A000000),
    ADP_TOPIC_SYSTEM_CLI_EXECUTE_CMD   = (ADP_TOPIC_SYSTEM | 0x0C000000),
};


#endif /* APP_CONFIG_H_ */
