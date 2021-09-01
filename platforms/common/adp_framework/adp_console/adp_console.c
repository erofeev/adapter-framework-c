/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include "adp_dispatcher.h"
#include "adp_logging.h"
#include "adp_uart.h"
#include "adp_console.h"


#define ADP_ASCII_LF                      0x0A
#define ADP_ASCII_CR                      0x0D
#define ADP_ASCII_BACKSPACE               0x08
#define ADP_ASCII_SPACE                   0x20

#ifdef ADP_CONSOLE_MODULE_NO_DEBUG
 #ifdef adp_log_d
  #undef  adp_log_d
 #endif
 #define adp_log_d(...)
#endif


char *adp_console_get_next_arg(const char *current_arg)
{
    while (*current_arg++ != 0) ;
    return (char*)current_arg;
}


int console_handler(uint32_t topic_id, void* data, uint32_t len)
{
    static char buffer[ADP_CONSOLE_MAX_CMD_SIZE];
    static unsigned char   * const argc    = (unsigned char*)&(buffer[0]);
    static          char         *cmd_line = &(buffer[1]);
    static          int            cmd_idx = 0;
    const           char        *in_stream = (const char*)data;

    while (len--) {
        const char ch = *in_stream++;
        // If cmd line already contains something
        if (cmd_idx) {
            // If 'enter' received
            if ((ch == ADP_ASCII_CR) || (ch == ADP_ASCII_LF)) {
                //
                // Cmd line is completed
                //
                if (cmd_idx) {
                    cmd_line[cmd_idx] = 0x00;
                    adp_log_d("CLI cmd[] argc = %d", cmd_line, *argc);
                    adp_topic_publish(ADP_TOPIC_CLI_EXECUTE_CMD, buffer, cmd_idx + 1 /* NUll */ + 1 /* argc */, ADP_TOPIC_PRIORITY_NORMAL);
                }

                // Reset cmd line
                cmd_idx = 0;
                cmd_line[0] = 0x00;
                *argc = 0;
                goto done;
            }
            // If backspace received
            if ((ch == ADP_ASCII_BACKSPACE) && (cmd_idx)) {
                cmd_idx--;
                adp_uart_putchar(ADP_ASCII_BACKSPACE);
                adp_uart_putchar(ADP_ASCII_SPACE);
                adp_uart_putchar(ADP_ASCII_BACKSPACE);
                goto done;
            }
        }

        // Prevent overflow
        if (cmd_idx == ADP_CONSOLE_MAX_CMD_SIZE - 1) {
            cmd_idx--;
            adp_uart_putchar(ADP_ASCII_BACKSPACE);
        }

        // Check validity of the symbol: it should be in range [0..Z]
        // Assemble cmd line
        if ((ch > 31) && (ch < 127)) {
            // Handle spaces
            if (ch == ADP_ASCII_SPACE) {
                if (!cmd_idx) {
                    // Skip spaces in the beginning
                    goto done;
                }
                if (cmd_line[cmd_idx] == ADP_ASCII_SPACE) {
                    // Skip multiple delimiters between words
                    goto done;
                }
                // Start of new argument detected
                cmd_line[cmd_idx++] = 0x00;
                *argc = *argc + 1;
            } else {
                cmd_line[cmd_idx++] = ch;
            }
        }

done:
        // Echo incoming symbol
        if (ADP_CONSOLE_ECHO_ENABLED) {
            adp_uart_putchar(ch);
        }
        if (!cmd_idx) {
            adp_uart_putchar('>');
        }
    }
    return ADP_RESULT_SUCCESS;
}


void adp_console_task(void* params)
{
    UNUSED_VAR(params);

    adp_dispatcher_handle_t dispatcher = (adp_dispatcher_handle_t)params;

    // Connect the topic to the dispatcher
    if (dispatcher) {
        adp_topic_register(dispatcher, ADP_TOPIC_CLI_INPUT_STREAM, "CLI.InputStream");
        adp_topic_register(dispatcher, ADP_TOPIC_CLI_EXECUTE_CMD , "CLI.ExecuteCmd");
    }
    adp_topic_subscribe(ADP_TOPIC_CLI_INPUT_STREAM, &console_handler, "ADP.CLI.SVC.Executor");

    while (1) {
        char c __attribute__ ((aligned (sizeof(void*)))) = adp_uart_getchar();
        if (c > 0) {
            adp_topic_publish(ADP_TOPIC_CLI_INPUT_STREAM, &c, sizeof(c), ADP_TOPIC_PRIORITY_NORMAL);
        }
    }
}
