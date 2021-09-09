/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_LOGGING__H_
#define ADAPTERS_ADP_LOGGING__H_

typedef enum {
    ADP_LOG_FATAL = 0,
    ADP_LOG_ERROR,
    ADP_LOG_INFO,
    ADP_LOG_DEBUG,
    ADP_LOG_DEBUG2,
} adp_log_level_t;

void adp_log_output(adp_log_level_t level, const char* fname, const char* function, const char* fmt,...);

#define adp_log(x, ...)          adp_log_output(ADP_LOG_INFO,   __FILE__, __FUNCTION__, x, ##__VA_ARGS__)
#define adp_log_d(x, ...)        adp_log_output(ADP_LOG_DEBUG,  __FILE__, __FUNCTION__, x, ##__VA_ARGS__)
#define adp_log_dd(x, ...)       adp_log_output(ADP_LOG_DEBUG2, __FILE__, __FUNCTION__, x, ##__VA_ARGS__)
#define adp_log_e(x, ...)        adp_log_output(ADP_LOG_ERROR,  __FILE__, __FUNCTION__, x, ##__VA_ARGS__)
#define adp_log_f(x, ...)        adp_log_output(ADP_LOG_FATAL,  __FILE__, __FUNCTION__, x, ##__VA_ARGS__)


#endif /* ADAPTERS_ADP_LOGGING__H_ */
