/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_MQTT__H_
#define ADAPTERS_ADP_MQTT__H_

#include "adp_dispatcher.h"

#define ADP_MQTT_BUFFER_SIZE         2048

typedef enum {
    ADP_MQTT_DO_INIT         = 0x00000001,
    ADP_MQTT_DO_CONNECT      = 0x00000002,
    ADP_MQTT_DO_SUBSCRIBE    = 0x00000003,
    ADP_MQTT_DO_PUBLISH      = 0x00000004,
    ADP_MQTT_DO_DISCONNECT   = 0x00000005,
} adp_mqtt_command_code_t;

typedef struct {
    adp_mqtt_command_code_t     command;
    uint32_t                     status;
    uint32_t                    subcode;
} adp_mqtt_status_t;

typedef struct {
    adp_mqtt_command_code_t     command;
} adp_mqtt_cmd_t;


typedef struct {
    union {
        adp_mqtt_status_t        status;         // The content of ADP_TOPIC_SYSTEM_MQTT_STATUS
        adp_mqtt_cmd_t          cmdline;         // The content of ADP_TOPIC_SYSTEM_MQTT_EXECUTE_CMD
    };
} adp_mqtt_topic_t;


adp_result_t adp_mqtt_initialize(adp_dispatcher_handle_t dispatcher);


#endif /* ADAPTERS_ADP_MQTT__H_ */
