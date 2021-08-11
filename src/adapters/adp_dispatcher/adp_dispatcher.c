/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_logging.h"


void adp_dispatcher_cycle(void)
{

    adp_log("-");
}


void adp_dispatcher_loop(void)
{
    while (1) {
        adp_dispatcher_cycle();
        adp_os_sleep(500);
    }
}

