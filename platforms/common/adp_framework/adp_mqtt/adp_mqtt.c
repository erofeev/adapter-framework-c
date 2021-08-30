/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
*/

#include "stdint.h"
#include "string.h"
#include "core_mqtt.h"

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_logging.h"
#include "adp_mqtt.h"


#ifdef ADP_MQTT_MODULE_NO_DEBUG
 #ifdef adp_log_d
  #undef  adp_log_d
 #endif
 #define adp_log_d(...)
#endif


MQTTContext_t      mqttContext;
TransportInterface_t transport;
MQTTFixedBuffer_t  fixedBuffer;
uint8_t            buffer[ADP_MQTT_BUFFER_SIZE];


uint32_t mqtt_getTimeStampMs()
{
    return adp_os_uptime_ms();
}


void mqtt_eventCallback(MQTTContext_t pContext, MQTTPacketInfo_t pPacketInfo, MQTTDeserializedInfo_t pDeserializedInfo)
{
    // Notify about event
    adp_log_e("TODO not implemented yet");
    adp_topic_publish(ADP_TOPIC_SYSTEM_MQTT_STATUS, NULL, 0, ADP_TOPIC_PRIORITY_HIGH);
}


adp_result_t mqtt_do_init(adp_mqtt_status_t *result, adp_mqtt_cmd_t* init_data)
{
    /*
        // Network send.
        int32_t networkSend( NetworkContext_t pContext, const void pBuffer, size_t bytes );
        // Network receive.
        int32_t networkRecv( NetworkContext_t pContext, void pBuffer, size_t bytes );
    */

    // Clear context.
    memset((void*)&mqttContext, 0x00, sizeof(MQTTContext_t));

    // Set transport interface members.
//    transport.pNetworkContext = &someTransportContext;
//    transport.send = networkSend;
//    transport.recv = networkRecv;

    // Set buffer members.
    fixedBuffer.pBuffer = buffer;
    fixedBuffer.size    = ADP_MQTT_BUFFER_SIZE;

    MQTTStatus_t core_mqtt_status = MQTT_Init(&mqttContext, &transport, mqtt_getTimeStampMs, (MQTTEventCallback_t)mqtt_eventCallback, &fixedBuffer);
    result->subcode = core_mqtt_status;

    if( core_mqtt_status != MQTTSuccess ) {
        result->status = ADP_RESULT_FAILED;
        adp_log_e("MQTT init result is %d", core_mqtt_status);
        return ADP_RESULT_FAILED;
    }

    result->status = ADP_RESULT_SUCCESS;

    return ADP_RESULT_SUCCESS;
}


int mqtt_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_cmd_t    *topic = (adp_mqtt_cmd_t*)data;
    adp_mqtt_status_t  result;

    adp_log_d("MQTT got cmd#%d to be processed", topic->command);

    if (topic->command == ADP_MQTT_DO_INIT) {
        mqtt_do_init(&result, topic);
    } else {
        adp_log_e("Unknown MQTT cmd #%d", topic->command);
        return ADP_RESULT_FAILED;
    }

    result.command = topic->command;
    adp_topic_publish(ADP_TOPIC_SYSTEM_MQTT_STATUS, &result, sizeof(adp_mqtt_status_t), ADP_TOPIC_PRIORITY_HIGH);

    return ADP_RESULT_SUCCESS;
}


adp_result_t adp_mqtt_initialize(adp_dispatcher_handle_t dispatcher)
{
    // Register topics
    adp_topic_register(dispatcher, ADP_TOPIC_SYSTEM_MQTT_STATUS,      "MQTT.Status");
    adp_topic_register(dispatcher, ADP_TOPIC_SYSTEM_MQTT_EXECUTE_CMD, "MQTT.ExecuteCmd");

    // Subscribe for commands
    adp_topic_subscribe(ADP_TOPIC_SYSTEM_MQTT_EXECUTE_CMD, &mqtt_cmd_handler, "MQTT.Executor");

    return ADP_RESULT_SUCCESS;
}
