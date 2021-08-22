/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef APP_TOPIC_LIST_H_
#define APP_TOPIC_LIST_H_


enum {
    ADP_TOPIC_SYSTEM                   = 0x80000000,
    ADP_TOPIC_SYSTEM_CLI_INPUT_STREAM  = (ADP_TOPIC_SYSTEM | 0x0A000000),
    ADP_TOPIC_SYSTEM_CLI_EXECUTE_CMD   = (ADP_TOPIC_SYSTEM | 0x0C000000),
};


#endif /* APP_TOPIC_LIST_H_ */
