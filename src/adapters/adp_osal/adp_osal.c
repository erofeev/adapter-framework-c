/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_logging.h"


void vApplicationIdleHook( void )
{
}

void vApplicationTickHook( void )
{
}

void vApplicationDaemonTaskStartupHook( void )
{
    adp_log("Ok");
}

void *adp_os_malloc(unsigned int size)
{
    return pvPortMalloc(size);
}

void adp_os_free(void* ptr)
{
    if (ptr)
        vPortFree(ptr);
}

int adp_os_uptime(void)
{
    return xTaskGetTickCount()/configTICK_RATE_HZ;
}

int adp_os_uptime_ms(void)
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

__attribute__((weak)) size_t xPortGetFreeHeapSize( void )
{
    adp_log("%s not supported", __FUNCTION__);
    return -1;
}

int adp_os_get_free_heap_size(void)
{
    return (int)xPortGetFreeHeapSize();
}

void adp_os_generic_thread( void* params )
{
    adp_os_start_task_t l_task = params;
    for( ;; )
    {
        l_task();
    }
}

void adp_os_sleep(int time_ms)
{
    vTaskDelay(pdMS_TO_TICKS(time_ms));
}

void adp_os_start_task(const char* task_name, adp_os_start_task_t task_body, int stack_size, int task_prio)
{
    char* auto_name = NULL;
    if (!task_name) {
        int size_of_name = sizeof(ADP_SYS_TASK_NAME_MASK);
        char *auto_name = (char*) adp_os_malloc(size_of_name);
        if (auto_name) {
            memcpy(auto_name, ADP_SYS_TASK_NAME_MASK, size_of_name);
            auto_name[size_of_name - 2] = (0x30 + task_prio) % 255;
        }
        task_name = auto_name;
    }
    xTaskCreate(&adp_os_generic_thread,
                 task_name,
                 stack_size,
                 task_body,
                 task_prio,
                 NULL );

    if (!auto_name) {
        adp_os_free(auto_name);
    }
}

void adp_os_start(void)
{
    vTaskStartScheduler();
}
