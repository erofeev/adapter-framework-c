/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_OSAL_ADP_OSAL_H_
#define ADAPTERS_ADP_OSAL_ADP_OSAL_H_


typedef void (*adp_os_start_task_t)(void);


void adp_os_start(adp_os_start_task_t task_to_run);
void adp_os_sleep(int time_ms);


#endif /* ADAPTERS_ADP_OSAL_ADP_OSAL_H_ */
