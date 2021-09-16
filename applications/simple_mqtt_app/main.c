/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdlib.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_console.h"
#include "adp_tcpip.h"
#include "adp_mqtt.h"
#include "adp_logging.h"
#include "adp_mem_table.h"

#include "app_console.h"
#include "app_mqtt.h"


void print_info(void* params)
{
    UNUSED_VAR(params);
    int interval = 1; // 1 minute
    char  *data;

    while(1) {
        adp_os_sleep(1000 * interval);
        adp_log("GenInfo: uptime is %d seconds", adp_os_uptime());

        // Print the whole DB
        data = "db\n";
 //       adp_topic_publish(ADP_TOPIC_CLI_INPUT_STREAM, data, strlen(data) + 1, ADP_TOPIC_PRIORITY_HIGH);

        // Print the memory trace DB
        data = "mem\n";
  //      adp_topic_publish(ADP_TOPIC_CLI_INPUT_STREAM, data, strlen(data) + 1, ADP_TOPIC_PRIORITY_HIGH);

        // ctx.name
        // ctx.notify_on_size
        // ctx.items_cnt ???
        //
        // adp_mem_push(ctx, data, len, ZeroCopy)
        // adp_mem_pop(ctx, data, &len)
        //
        //
        //  adp_memdb_put (ctx, ""
        // ctx.format         = "%s%d";
        // ctx.row_count      =
        // ctx.total_max_size =
        // while (ctx.id != NULL) {
        //    ctx.id = adp_row_get(ctx, "%S%S%d", "Network", "Configured", &configured);
        // }
        // adp_row_get(
        // row_ptr = adp_mem_table_row_add(ctx,
        // adp_mem_list_push(ctx, row_ptr, sizeof(row_ptr));
        //


        static adp_mem_table_t *table = NULL;
        if (!table) {
            table = adp_os_malloc(sizeof(adp_mem_table_t));
            table->format   = "[%d] [%s.%s] = [%d]";
            table->name     = "Table#1";
            table->row_size = 0;
        }

        adp_mem_table_row_t t = adp_mem_table_row_add(table, adp_os_uptime_ms(), "Network", "SSID", 1);

        ADP_SIGNED_INT32 timestamp, i32;
        ADP_STRING       str1;
        ADP_STRING       str2;
        adp_mem_table_row_get(table, t, &timestamp, &str1, &str2, &i32);

        adp_log("Getting row");
        adp_log(table->format, timestamp, str1, str2, i32);
        adp_mem_table_row_del(table, t);


    }
}

// Handling: Net is UP or DOWN
int net_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    static adp_ipnet_status_t prev_status = ADP_IPNET_STACK_NA;
    adp_ipnet_status_t status = *(adp_ipnet_status_t*)data;

    if (status == prev_status) {
        // DHCP renew happened
        return ADP_RESULT_SUCCESS;
    }
    adp_log("Status: Network is %s", (status == ADP_IPNET_STACK_DOWN) ? "down" : "up");

    switch (status) {
    case ADP_IPNET_STACK_UP:
        {
            // Nothing to do
        }
        break;
    case ADP_IPNET_STACK_DOWN:
        {
            // Nothing to do
        }
        break;
    default:
        break;
    }

    prev_status = status;
    return ADP_RESULT_SUCCESS;
}


int main(void) {
    adp_log("Build date [%s %s]", __DATE__, __TIME__);
    adp_log("Creating the World");

    // Example of user task creation
    adp_os_start_task("APP Info-print", &print_info, 128, 0, NULL);

    // Run console and subscribe on the CLI cmd execution topic
    adp_dispatcher_handle_t low_prio_dispatcher = adp_dispatcher_create("CLI", 0, 80);
    adp_os_start_task("APP Console", &adp_console_task, 128, 0, low_prio_dispatcher);
    adp_topic_subscribe(ADP_TOPIC_CLI_EXECUTE_CMD, &app_cmd_handler, "USER app_cmd_handler");

    // Initialize TCP/IP stack
    adp_dispatcher_handle_t network_dispatcher = adp_dispatcher_create("IPNET", adp_os_get_max_prio() - 2, 70);
    adp_ipnet_initialize(network_dispatcher);

    // Initialize MQTT
    adp_dispatcher_handle_t mqtt_dispatcher    = adp_dispatcher_create("MQTT",  adp_os_get_max_prio() - 3, 70);
    adp_mqtt_initialize(mqtt_dispatcher);

    // Subscribe
    adp_topic_subscribe(ADP_TOPIC_IPNET_STATUS, &net_status_handler, "USER Net Up/Down");

    adp_os_start();

    return EXIT_SUCCESS;
}

