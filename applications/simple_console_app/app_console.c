/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <string.h>

#include "adp_dispatcher.h"
#include "adp_logging.h"

#include "app_console.h"


int app_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_log("APP CLI CMD executor, cmd = [%s]", data);

    if (strcmp("db", data) == 0) {
        adp_dispatcher_db_print(NULL);
    }

    return ADP_RESULT_SUCCESS;
}
