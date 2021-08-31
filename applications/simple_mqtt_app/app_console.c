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
    adp_log("\t db        - print DB table");
    adp_log("\t netstat   - print network runtime stats");
    adp_log("\t os        - print OS runtime stats");
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

HANDLING_CMD(NETSTAT)
{
    extern void FreeRTOS_netstat( void );
    FreeRTOS_netstat();
}

int app_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    const char *cmd = data;
    int  argc = *cmd;
    const char* cmd_name = ++cmd;

    if (strcmp(cmd_name, "db"  ) == 0) {
        HANDLE_CMD(DB, cmd_name, argc);
    } else
    if (strcmp(cmd_name, "os"  ) == 0) {
        HANDLE_CMD(OS, cmd_name, argc);
    } else
    if (strcmp(cmd_name, "netstat"  ) == 0) {
        HANDLE_CMD(NETSTAT, cmd_name, argc);
    } else {
        HANDLE_CMD(HELP, cmd_name, argc);
    }

    return ADP_RESULT_SUCCESS;
}
