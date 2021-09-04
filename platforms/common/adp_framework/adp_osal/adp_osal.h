/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_OSAL_H_
#define ADAPTERS_ADP_OSAL_H_

#include <stdint.h>

#define UNUSED_VAR(x)              (void)x
#define ADP_WEAK                   __attribute__((weak))

typedef enum {
    ADP_RESULT_FAILED              = -32767,
    ADP_RESULT_TIMEOUT,
    ADP_RESULT_INVALID_PARAMETER,
    ADP_RESULT_NO_SPACE_LEFT,
    ADP_RESULT_SUCCESS = 0,
} adp_result_t;

#define ADP_ASSERT(A, MSG)         do { if (!A) { adp_log_f(MSG); adp_os_abnormal_stop();} } while (0)


typedef void (adp_os_start_task_t)(void* );
typedef void* adp_os_queue_handle_t;


// Task handling
void adp_os_start(void);

adp_result_t adp_os_start_task(const char* task_name, adp_os_start_task_t task_body, uint32_t stack_size, uint32_t task_prio, void* user_data);

char* adp_os_get_task_name(void);

void adp_os_abnormal_stop(void);


// Time management
uint32_t adp_os_uptime(void);

uint32_t adp_os_uptime_ms(void);

uint32_t adp_os_rand(void);

void adp_os_sleep(uint32_t time_ms);


// Memory management
int adp_os_get_free_heap_size(void);

void *adp_os_malloc(uint32_t size);

void adp_os_free(void* ptr);


// Queue management
adp_os_queue_handle_t adp_os_queue_create(uint32_t queue_length, uint32_t item_size);

adp_result_t adp_queue_receive(adp_os_queue_handle_t queue, void * const item, uint32_t timeout_ms);

adp_result_t adp_os_queue_put(adp_os_queue_handle_t, const void *item);

int adp_queue_msg_total(adp_os_queue_handle_t queue);

int adp_queue_msg_waiting(adp_os_queue_handle_t queue);


// Misc & Debug Util
int adp_os_get_max_prio(void);

void adp_os_get_tasks_rtstats(char *buffer);

void adp_os_get_tasks_list(char *buffer);


#endif /* ADAPTERS_ADP_OSAL_H_ */
