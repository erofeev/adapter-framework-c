/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADP_CONSOLE_H_
#define ADP_CONSOLE_H_


#define ADP_CONSOLE_ECHO_ENABLED       0


enum {
    ADP_TOPIC_SYSTEM                   = 0x80000000,
    ADP_TOPIC_SYSTEM_CLI_INPUT_STREAM  = (ADP_TOPIC_SYSTEM | 0x0A000000),
    ADP_TOPIC_SYSTEM_CLI_EXECUTE_CMD   = (ADP_TOPIC_SYSTEM | 0x0C000000),
};


void adp_console_task(void* params);

char *adp_console_get_next_arg(const char *current_arg);


#endif /* ADP_CONSOLE_H_ */
