/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADP_CONSOLE_H_
#define ADP_CONSOLE_H_


#define ADP_CONSOLE_ECHO_ENABLED       0



void adp_console_task(void* params);

char *adp_console_get_next_arg(const char *current_arg);


#endif /* ADP_CONSOLE_H_ */
