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



int main(void)
{
    adp_log("Creating the World");


	adp_os_start(&adp_dispatcher_loop);

	return EXIT_SUCCESS;
}
