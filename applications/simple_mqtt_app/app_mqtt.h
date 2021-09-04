/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef APP_MQTT_H_
#define APP_MQTT_H_


#include <stdint.h>

int app_net_status_handler(uint32_t topic_id, void* data, uint32_t len);

int app_net_cmd_status_handler(uint32_t topic_id, void* data, uint32_t len);

int app_mqtt_cmd_status_handler(uint32_t topic_id, void* data, uint32_t len);

int app_mqtt_socket_activity_handler(uint32_t topic_id, void* data, uint32_t len);

#endif /* APP_MQTT_H_ */
