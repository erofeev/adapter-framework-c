/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_LOGGING__H_
#define ADAPTERS_ADP_LOGGING__H_

#include "log.h"

#define adp_log(...)   log_info (__VA_ARGS__)
#define adp_log_e(...) log_error(__VA_ARGS__)
#define adp_log_f(...) log_fatal(__VA_ARGS__)
#define adp_log_d(...) log_debug(__VA_ARGS__)

#endif /* ADAPTERS_ADP_LOGGING__H_ */
