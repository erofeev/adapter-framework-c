/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <string.h>

#include "adp_dispatcher.h"
#include "adp_logging.h"

#include "app_console.h"


#define HANDLE_CMD(CMD_NAME, data, length)     cmd_##CMD_NAME(data, argc)
#define HANDLING_CMD(CMD_NAME)                 void cmd_##CMD_NAME(const char *data, int argc)


HANDLING_CMD(HELP)
{
    adp_log("Supported commands:");
    adp_log("\t mem                        - memory allocation trace");
    adp_log("\t db                         - print DB table");
    adp_log("\t netstat                    - print network runtime stats");
    adp_log("\t os                         - print OS runtime stats");
}

ADP_WEAK
void adp_os_mem_trace_print()
{
    adp_log_e("ADP_MEMORY_ALLOC_FREE_TRACE_ENABLED should be enabled first");
}

HANDLING_CMD(MEM)
{
    adp_os_mem_trace_print();
}

HANDLING_CMD(DB)
{
    adp_dispatcher_db_print(NULL);
}

HANDLING_CMD(OS)
{
    uint32_t uptime = adp_os_uptime_ms();
    char *buffer = adp_os_malloc(256);
    if (buffer) {
        memset(buffer, 0x00, 256);
        adp_os_get_tasks_list(buffer);
        adp_log("\n\r\n\rUptime - %lu.%03d seconds\n%s", uptime/1000, uptime%1000, buffer);
        adp_os_free(buffer);
    }
}


HANDLING_CMD(A)
{
main2();
adp_log("EXITED");
}

int app_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    const char *cmd = data;
    int  argc = *cmd;
    const char* cmd_name = ++cmd;

    if (strcmp(cmd_name, "mem"  ) == 0) {
        HANDLE_CMD(MEM, cmd_name, argc);
    } else
    if (strcmp(cmd_name, "db"  ) == 0) {
        HANDLE_CMD(DB, cmd_name, argc);
    } else
    if (strcmp(cmd_name, "os"  ) == 0) {
        HANDLE_CMD(OS, cmd_name, argc);
    } else
    if (strcmp(cmd_name, "a"  ) == 0) {
        HANDLE_CMD(A, cmd_name, argc);
    } else {
        HANDLE_CMD(HELP, cmd_name, argc);
    }

    return ADP_RESULT_SUCCESS;
}
