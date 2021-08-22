/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <string.h>

#include "adp_dispatcher.h"
#include "adp_logging.h"

#include "app_console.h"


#define HANDLE_CMD(CMD_NAME, data, length)     cmd_##CMD_NAME(data, length)
#define HANDLING_CMD(CMD_NAME)                 void cmd_##CMD_NAME(const char *data, int length)


HANDLING_CMD(HELP)
{
    adp_log("Supported commands:");
    adp_log("\t db - print DB table");
}

HANDLING_CMD(DB)
{
    adp_dispatcher_db_print(NULL);
}

int app_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    ADP_ASSERT(data,"Null pointer instead of cmd");
    ADP_ASSERT(len, "Cmd length = 0!");

    const char *cmd = data;
    int  argc = *cmd;
    const char* cmd_name = ++cmd;

    adp_log_d("APP CLI CMD executor, cmd = [%s] argc = %d", cmd_name, argc);

    if (strcmp(cmd_name, "db"  ) == 0) {
        HANDLE_CMD(DB, data, len);
    } else
    if (strcmp(cmd_name, "help") == 0) {
        HANDLE_CMD(HELP, data, len);
    }

    return ADP_RESULT_SUCCESS;
}
