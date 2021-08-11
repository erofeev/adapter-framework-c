/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_OSAL_ADP_OSAL_H_
#define ADAPTERS_ADP_OSAL_ADP_OSAL_H_

#include "FreeRTOS.h"
#include "task.h"

#define ADP_SYS_TASK_NAME_MASK     "ADP-x"


typedef void (*adp_os_start_task_t)(void);


void adp_os_start(void);

int  adp_os_uptime(void);

int  adp_os_uptime_ms(void);

int  adp_os_get_max_prio(void);

int  adp_os_get_free_heap_size(void);

void adp_os_start_task(const char* task_name, adp_os_start_task_t task_body, int stack_size, int task_prio);

char* adp_os_get_task_name(void);

void adp_os_sleep(int time_ms);

void *adp_os_malloc(unsigned int size);

void adp_os_free(void* ptr);

#endif /* ADAPTERS_ADP_OSAL_ADP_OSAL_H_ */
