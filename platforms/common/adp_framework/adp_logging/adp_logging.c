/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include "adp_osal.h"
#include "adp_logging.h"


#ifdef ADP_LOG_USE_COLOR
    #define ADP_LOG_COLOR_GRAY              "\x1b[94m"
    #define ADP_LOG_COLOR_DARK_GRAY         "\x1b[90m"
    #define ADP_LOG_COLOR_WHITE             "\x1b[37m"
    #define ADP_LOG_COLOR_BRIGHT_WHITE      "\x1b[0m"
    #define ADP_LOG_COLOR_BRIGHT_RED        "\x1b[31m"
#else
    #define ADP_LOG_COLOR_GRAY              ""
    #define ADP_LOG_COLOR_DARK_GRAY         ""
    #define ADP_LOG_COLOR_WHITE             ""
    #define ADP_LOG_COLOR_BRIGHT_WHITE      ""
    #define ADP_LOG_COLOR_BRIGHT_RED        ""
#endif

typedef struct {
    const char*           name;
    const char*          color;
} adp_log_settings_t;

adp_log_settings_t log_settings_s[] = {
        { /* FATAL */
                .color = ADP_LOG_COLOR_BRIGHT_RED,
                .name  = "FATAL",
        },
        { /* ERROR */
                .color = ADP_LOG_COLOR_BRIGHT_RED,
                .name  = "ERROR",
        },
        { /* INFO */
                .color = ADP_LOG_COLOR_BRIGHT_WHITE,
                .name  = "INFO",
        },
        { /* DEBUG */
                .color = ADP_LOG_COLOR_GRAY,
                .name  = "DEBUG",
        },
        { /* DEEP DEBUG */
                .color = ADP_LOG_COLOR_DARK_GRAY,
                .name  = "DEEP_DEBUG",
        },
};

void adp_log_output(adp_log_level_t level, const char* fname, const char* function, const char* fmt,...)
{
    static adp_os_mutex_t locker = NULL;
    if (!locker)
        locker = adp_os_mutex_create();

    char* tname = adp_os_get_task_name();
    if (!tname)
        tname = "---";

    uint32_t ms = adp_os_uptime_ms();
    uint32_t days = (ms / (1000 * 60 * 60 * 24));
    uint32_t hr   = (ms / (1000 * 60 * 60)) % 24;
    uint32_t min  = (ms / (1000 * 60)) % 60;
    uint32_t sec  = (ms / 1000) % 60;
    ms = ms % 1000;

    adp_log_settings_t *set = &log_settings_s[level];

    adp_os_mutex_take(locker);
    va_list args;
    va_start(args, fmt);
   //         1  2      3   4    5     6  7  8   9 10  11      1                2    3    4    5   6      7
    printf("%s%d days %02d:%02d:%02d.%03d %s%-6s %s[%s->%s]%s ", ADP_LOG_COLOR_WHITE, days, hr, min, sec, ms,
                                                              set->color, set->name,
                                                              ADP_LOG_COLOR_DARK_GRAY, tname, function, set->color);
    vprintf(fmt, args);
    printf("%s\n", ADP_LOG_COLOR_BRIGHT_WHITE);
    va_end(args);
    fflush(stdout);
    adp_os_mutex_give(locker);
}
