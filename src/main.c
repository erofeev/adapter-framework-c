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



int main(void) {
    adp_log("Creating the World");

    for (int i = 0; i < 2 /*configMAX_PRIORITIES*/; ++i) {
        adp_os_start_task(&adp_dispatcher_loop, 512, i);
    }

    adp_os_start();

    return EXIT_SUCCESS;
}
