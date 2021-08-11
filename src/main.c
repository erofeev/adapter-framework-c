/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_logging.h"

void task_print_info(void)
{
    adp_os_sleep(1000 * 10);
    adp_log("uptime is %d seconds", adp_os_uptime());
}

int main(void) {
    adp_log("Creating the World");

    adp_os_start_task("Info-print", &task_print_info, 512, 2);
    adp_os_start_task("USER-6"    , &adp_dispatcher , 512, 6);

    adp_os_start_task(NULL, &adp_dispatcher, 512, 5);

    adp_os_start();

    return EXIT_SUCCESS;
}
