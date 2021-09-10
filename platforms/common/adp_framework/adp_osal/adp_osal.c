/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "adp_osal.h"
#include "adp_logging.h"


#ifdef ADP_OS_MODULE_NO_DEBUG
    #undef  adp_log_d
    #undef  adp_log_dd
    #define adp_log_d(...)
    #define adp_log_dd(...)
#endif


#ifdef ADP_MEMORY_ALLOC_FREE_TRACE_ENABLED


typedef struct {
    const char     *caller_name;
    uint32_t               line;
    uint32_t          timestamp;
    void               *mem_ptr;
    uint32_t               size;
} adp_mem_usage_db_t;

adp_mem_usage_db_t  mem_db[ADP_MEMORY_ALLOC_FREE_TRACE_DB_SIZE] = {0};
uint32_t            peak         = 0;
uint32_t            total_peak   = 0;
adp_os_mutex_t      os_mem_mutex = NULL;

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
void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName )
{
    adp_log_e("Stack overflow at %s", pcTaskName);
    ADP_ASSERT(0, "Aborted");
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


#ifdef ADP_MEMORY_ALLOC_FREE_TRACE_ENABLED

void *adp_os_malloc_trace(uint32_t size, const char* caller_name, uint32_t line_number)
{
    uint32_t t = adp_os_uptime_ms();
    void *ptr = pvPortMalloc(size);
    if (!os_mem_mutex) {
        os_mem_mutex = adp_os_mutex_create();
    }
    adp_os_mutex_take(os_mem_mutex);
    peak += size;
    for (int i = 0; i < ADP_MEMORY_ALLOC_FREE_TRACE_DB_SIZE; i++) {
        if (!mem_db[i].mem_ptr) {
            mem_db[i].mem_ptr     = ptr;
            mem_db[i].timestamp   = t;
            mem_db[i].size        = size;
            mem_db[i].caller_name = caller_name;
            mem_db[i].line        = line_number;
            adp_os_mutex_give(os_mem_mutex);
            return ptr;
        }
    }
    adp_os_mutex_give(os_mem_mutex);
    adp_log_e("Mem trace DB is full");
    return ptr;
}

void  adp_os_free_trace(void* ptr)
{
    if (!ptr)
        return;
    adp_os_mutex_take(os_mem_mutex);
    for (int i = 0; i < ADP_MEMORY_ALLOC_FREE_TRACE_DB_SIZE; i++) {
        if (mem_db[i].mem_ptr == ptr) {
            mem_db[i].mem_ptr = NULL;
            vPortFree(ptr);
            if (peak > total_peak)
                total_peak = peak;
            peak -= mem_db[i].size;
            adp_os_mutex_give(os_mem_mutex);
            return;
        }
    }
    adp_os_mutex_give(os_mem_mutex);
    adp_log_e("Mem trace - free() on unallocated pointer");
}

void  adp_os_mem_trace_print()
{
    int      k   = 0;
    uint32_t sum = 0;
    adp_log("%3s %20s %40s:%4s  %20s  %s", "#", "Timestamp", "Function", "Line", "Size", "Pointer");
    adp_os_mutex_take(os_mem_mutex);
    for (int i = 0; i < ADP_MEMORY_ALLOC_FREE_TRACE_DB_SIZE; i++) {
        if (mem_db[i].mem_ptr) {
            k++;
            sum += mem_db[i].size;
            adp_log("%03d %20d %40s:%4d  %20d  0x%x", k, mem_db[i].timestamp, mem_db[i].caller_name, mem_db[i].line, mem_db[i].size, mem_db[i].mem_ptr);
        }
    }
    adp_log("ADP modules, heap alloc'd total: %d Max used peak: %d", sum, total_peak);
    adp_os_mutex_give(os_mem_mutex);
}
#else

void *adp_os_malloc(uint32_t size)
{
    void *ptr = pvPortMalloc(size);
    adp_log_d("Mem alloc 0x%x", ptr, size);
    return ptr;
}

void adp_os_free(void* ptr)
{
    adp_log_d("Mem free 0x%x", ptr);
    if (ptr)
        vPortFree(ptr);
}
#endif

uint32_t adp_os_uptime(void)
{
    return xTaskGetTickCount()/configTICK_RATE_HZ;
}

uint32_t adp_os_uptime_ms(void)
{
    return xTaskGetTickCount()/(configTICK_RATE_HZ/1000);
}

adp_os_timer_t adp_os_timer_start(uint32_t timeout_ms, int auto_reload, adp_os_timer_cb_t callback)
{
    if (!timeout_ms)
        return NULL;

    adp_os_timer_t t = (adp_os_timer_t)xTimerCreate("", timeout_ms * portTICK_PERIOD_MS,
                                                    (UBaseType_t)auto_reload, NULL,
                                                    (TimerCallbackFunction_t)callback);
    if (!t) {
        ADP_ASSERT(t, "Unable to create a timer");
        return NULL;
    }

    if( xTimerStart(t, 0) != pdPASS ) {
        ADP_ASSERT(t, "Unable to start a timer");
        return NULL;
    }

    return t;
}

adp_result_t adp_os_timer_stop(adp_os_timer_t timer_obj)
{
    BaseType_t result = xTimerStop(timer_obj, 100);
    if (result != pdPASS ) {
        adp_log_e("Unable to stop timer");
        return ADP_RESULT_TIMEOUT;
    }
    return ADP_RESULT_SUCCESS;
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

uint32_t adp_os_get_total_heap_size(void)
{
    return configTOTAL_HEAP_SIZE;
}

uint32_t adp_os_get_free_heap_size(void)
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

adp_os_mutex_t adp_os_mutex_create(void)
{
    adp_os_mutex_t mutex = (adp_os_mutex_t)xSemaphoreCreateMutex();
    ADP_ASSERT(mutex, "Unable to create mutex");
    return mutex;
}

void adp_os_mutex_take(adp_os_mutex_t mutex)
{
    xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY);
}

void adp_os_mutex_give(adp_os_mutex_t mutex)
{
    xSemaphoreGive((SemaphoreHandle_t)mutex);
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
