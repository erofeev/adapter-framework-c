/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_MQTT_AGENT__H_
#define ADAPTERS_ADP_MQTT_AGENT__H_

#include <adp_mqtt.h>


typedef struct {
    char                                 *name;
    uint16_t                              port;
    char                             *hostname;
    uint8_t                          ip_octet1;
    uint8_t                          ip_octet2;
    uint8_t                          ip_octet3;
    uint8_t                          ip_octet4;
    uint16_t                    ack_timeout_ms;
    uint16_t                     clean_session;
    uint16_t                keep_alive_seconds;
    const char                      *client_id;
    const char                       *username;
    const char                       *password;
    uint16_t                  number_of_topics;
    adp_mqtt_topic_filter_t            topics[];
} adp_mqtt_client_t;


void adp_mqtt_agent_start(adp_mqtt_client_t *client);


#endif /* ADAPTERS_ADP_MQTT_AGENT__H_ */
