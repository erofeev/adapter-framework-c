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
    ADP_MQTT_DO_CONNECT      = 0x00000001,
    ADP_MQTT_DO_SUBSCRIBE    = 0x00000002,
    ADP_MQTT_DO_PUBLISH      = 0x00000003,
    ADP_MQTT_DO_DISCONNECT   = 0x00000004,
} adp_mqtt_command_code_t;

typedef struct adp_mqtt_session_s adp_mqtt_session_t;

// The content of ADP_TOPIC_MQTT_STATUS
typedef struct {
    adp_mqtt_command_code_t     command;
    uint32_t                     status;
    uint32_t                    subcode;
    void                    *session_id;
} adp_mqtt_cmd_status_t;

// The content of ADP_TOPIC_MQTT_EXECUTE_CMD - ADP_MQTT_DO_CONNECT
typedef struct {
    uint16_t             ack_timeout_ms;
    uint16_t              clean_session;
    uint16_t         keep_alive_seconds;
    const char               *client_id;
    const char                *username;
    const char                *password;
} adp_mqtt_cmd_connect_t;

typedef struct {
    int                             QoS;
    const char            *topic_filter;
} adp_mqtt_topic_filter_t;

// The content of ADP_TOPIC_MQTT_EXECUTE_CMD - ADP_MQTT_DO_SUBSCRIBE
typedef struct {
    uint8_t           number_of_filters;
    adp_mqtt_topic_filter_t    *filters;
} adp_mqtt_cmd_subscribe_t;

// The content of ADP_TOPIC_MQTT_EXECUTE_CMD
typedef struct {
    adp_mqtt_command_code_t         command;
    void                        *session_id;
    union {
        adp_mqtt_cmd_connect_t      connect;
        adp_mqtt_cmd_subscribe_t  subscribe;
    };
} adp_mqtt_cmd_t;

// The content of ADP_TOPIC_MQTT_INCOMING_TOPIC
typedef struct {
    void                        *session_id;
    size_t                  topic_name_size;
    size_t                     payload_size;
    const char                  *topic_name;
    uint8_t                        *payload;
} adp_mqtt_received_topic_t;

adp_mqtt_session_t* adp_mqtt_session_alloc(void* socket);

void adp_mqtt_session_free(adp_mqtt_session_t* session);

adp_result_t adp_mqtt_initialize(adp_dispatcher_handle_t dispatcher);


#endif /* ADAPTERS_ADP_MQTT__H_ */
