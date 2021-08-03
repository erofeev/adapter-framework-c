/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

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
    adp_log("[%s]", __FUNCTION__);
}


void vTaskCode( void* params )
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

void adp_os_start(adp_os_start_task_t task_to_run)
{
    xTaskCreate( &vTaskCode,
                 "osal",
                 configMINIMAL_STACK_SIZE,
                 task_to_run,
                 tskIDLE_PRIORITY,
                 NULL );

    vTaskStartScheduler();
}
