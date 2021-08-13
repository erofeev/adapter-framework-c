/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_OSAL_ADP_OSAL_H_
#define ADAPTERS_ADP_OSAL_ADP_OSAL_H_

#include <stdint.h>


#define ADP_WEAK                   __attribute__((weak))
#define ADP_RESULT_SUCCESS         0
#define ADP_RESULT_FAILED         -1
#define ADP_ASSERT(A, MSG)         do { if (!A) { adp_log_f(MSG); while(1); } } while (0)


typedef void (adp_os_start_task_t)(void* );
typedef void* adp_os_queue_handle_t;


// Task handling
void adp_os_start(void);

int adp_os_start_task(const char* task_name, adp_os_start_task_t task_body, uint32_t stack_size, uint32_t task_prio, void* user_data);

char* adp_os_get_task_name(void);


// Time management
uint32_t adp_os_uptime(void);

uint32_t adp_os_uptime_ms(void);

void adp_os_sleep(uint32_t time_ms);


// Memory management
int adp_os_get_free_heap_size(void);

void *adp_os_malloc(uint32_t size);

void adp_os_free(void* ptr);


// Queue management
adp_os_queue_handle_t adp_os_queue_create(uint32_t queue_length, uint32_t item_size);

int adp_queue_receive(adp_os_queue_handle_t queue, void * pvBuffer, uint32_t timeout_ms);

int adp_queue_msg_total(adp_os_queue_handle_t queue);

int adp_queue_msg_waiting(adp_os_queue_handle_t queue);


#endif /* ADAPTERS_ADP_OSAL_ADP_OSAL_H_ */
