/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "adp_osal.h"
#include "adp_logging.h"


#ifdef ADP_OS_MODULE_NO_DEBUG
 #ifdef adp_log_d
  #undef  adp_log_d
 #endif
 #define adp_log_d(...)
#endif


ADP_WEAK
void vApplicationIdleHook( void )
{
}

ADP_WEAK
void vApplicationTickHook( void )
{
}

ADP_WEAK
void vApplicationDaemonTaskStartupHook( void )
{
    adp_log("OS started");
}

ADP_WEAK
BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
    *(pulNumber) = adp_os_rand();
    return pdTRUE;
}

ADP_WEAK
size_t xPortGetFreeHeapSize(void)
{
    adp_log("%s not supported", __FUNCTION__);
    return ADP_RESULT_FAILED;
}

uint32_t adp_os_rand()
{
    static int i = 0;
    if (!i) { srand(time(NULL)); i = 1; }
    uint32_t r = rand();
    adp_log_d("RND is 0x%x", r);
    return r;
}

void *adp_os_malloc(uint32_t size)
{
    return pvPortMalloc(size);;
}

void adp_os_free(void* ptr)
{
    if (ptr)
        vPortFree(ptr);
}

uint32_t adp_os_uptime(void)
{
    return xTaskGetTickCount()/configTICK_RATE_HZ;
}

uint32_t adp_os_uptime_ms(void)
{
    return xTaskGetTickCount()/(configTICK_RATE_HZ/1000);
}
int adp_os_get_max_prio(void)
{
    return configMAX_PRIORITIES;
}

char* adp_os_get_task_name(void)
{
    if (uxTaskGetNumberOfTasks())
        return pcTaskGetName(NULL);
    else
        return NULL;
}

int adp_os_get_free_heap_size(void)
{
    return (int)xPortGetFreeHeapSize();
}

void adp_os_get_tasks_rtstats(char *buffer)
{
    vTaskGetRunTimeStats(buffer);
}

void adp_os_get_tasks_list(char *buffer)
{
    vTaskList(buffer);
}

void adp_os_sleep(uint32_t time_ms)
{
    vTaskDelay(pdMS_TO_TICKS(time_ms));
}

adp_os_queue_handle_t adp_os_queue_create(uint32_t queue_length, uint32_t item_size)
{
    QueueHandle_t queue = xQueueCreate(queue_length, item_size);
    return (adp_os_queue_handle_t)queue;
}

adp_result_t adp_queue_receive(adp_os_queue_handle_t queue, void * const item, uint32_t timeout_ms)
{
    BaseType_t result = xQueueReceive(queue, item, (TickType_t) timeout_ms * portTICK_PERIOD_MS);
    if (errQUEUE_EMPTY == result) {
        return ADP_RESULT_TIMEOUT;
    }
    if (pdPASS != result) {
        return ADP_RESULT_FAILED;
    }
    return ADP_RESULT_SUCCESS;
}

adp_result_t adp_os_queue_put(adp_os_queue_handle_t queue, const void *item)
{
    BaseType_t result = xQueueSend(queue, item, ADP_OS_DEFAULT_QUEUE_PUT_TIMEOUT_MS );

    if (result == errQUEUE_FULL) {
        return ADP_RESULT_NO_SPACE_LEFT;
    }
    if (result != pdPASS) {
        return ADP_RESULT_FAILED;
    }
    return ADP_RESULT_SUCCESS;
}

int adp_queue_msg_total(adp_os_queue_handle_t queue)
{
    int total;
    taskENTER_CRITICAL();
    {
        total = (int)uxQueueSpacesAvailable(queue) + (int)uxQueueMessagesWaiting(queue);
    }
    taskEXIT_CRITICAL();
    return total;
}

int adp_queue_msg_waiting(adp_os_queue_handle_t queue)
{
    return (int)uxQueueMessagesWaiting(queue);
}

int adp_queue_space_available(adp_os_queue_handle_t queue)
{
    return (int)uxQueueSpacesAvailable(queue);
}

void adp_os_abnormal_stop(void)
{
    adp_log_e("Fatal error! Rebooting...");
#ifdef RELEASE
    adp_os_reboot();
    while(1);
#else
    vTaskSuspendAll();
    while(1);
#endif
}

adp_result_t adp_os_start_task(const char* task_name, adp_os_start_task_t task_body, uint32_t stack_size, uint32_t task_prio, void* user_data)
{
    int result = xTaskCreate(
                (TaskFunction_t)task_body,
                 task_name,
                 stack_size,
                 user_data,
                 task_prio,
                 NULL );

    if (pdPASS == result) {
        return ADP_RESULT_SUCCESS;
    }
    return ADP_RESULT_FAILED;
}

void adp_os_start(void)
{
    vTaskStartScheduler();
}
