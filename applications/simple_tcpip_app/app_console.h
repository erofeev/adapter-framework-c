/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef APP_CONSOLE_H_
#define APP_CONSOLE_H_


#include <stdint.h>


int app_cmd_handler(uint32_t topic_id, void* data, uint32_t len);


#endif /* APP_CONSOLE_H_ */
