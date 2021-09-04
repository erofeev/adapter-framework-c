/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
*/

#include "stdint.h"
#include "string.h"
#include "core_mqtt.h"
#include "transport_interface.h"

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

typedef struct NetworkContext {
     void* socket;
     void* tlsContext;
 } adp_network_context_t;

 struct adp_mqtt_session_s {
      MQTTContext_t             context;
      MQTTConnectInfo_t         connect_info;
      MQTTSubscribeInfo_t    *subscribe_info;
 };

 adp_mqtt_session_t *s_session_db[ADP_MQTT_SESSIONS_MAX_NUMBER] = {0};

void mqtt_eventCallback(MQTTContext_t *pContext, MQTTPacketInfo_t *pPacketInfo, MQTTDeserializedInfo_t *pDeserializedInfo)
{
    // Notify about event
    adp_log_e("EVENT cb - TODO not implemented yet 0x%x 0x%x 0x%x", pContext, pPacketInfo, pDeserializedInfo);
    adp_log_e("EVENT cb conn status : %d", pContext->connectStatus);

    if (pDeserializedInfo->deserializationResult != MQTTSuccess) {
        adp_log_d("Deserialization error [%s]", MQTT_Status_strerror(pDeserializedInfo->deserializationResult));
        return;
    }
    adp_log(" T [%s], P [%s]", pDeserializedInfo->pPublishInfo->pTopicName, pDeserializedInfo->pPublishInfo->pPayload);
/*    size_t event_size = sizeof(adp_mqtt_received_topic_t ) + pDeserializedInfo->pPublishInfo->topicNameLength + pDeserializedInfo->pPublishInfo->payloadLength + 2;
    adp_mqtt_received_topic_t *topic = adp_os_malloc(event_size);
    ADP_ASSERT(topic, "Unable to store incoming data");

    adp_log_d(" T [%s], P [%s]", pDeserializedInfo->pPublishInfo->pTopicName, pDeserializedInfo->pPublishInfo->pPayload);
    topic->session_id      = pContext;
    topic->topic_name_size = pDeserializedInfo->pPublishInfo->topicNameLength + 1;
    topic->payload_size    = pDeserializedInfo->pPublishInfo->payloadLength + 1;
    memcpy(topic + sizeof(adp_mqtt_received_topic_t), pDeserializedInfo->pPublishInfo->pTopicName, topic->topic_name_size);
    memcpy(topic + sizeof(adp_mqtt_received_topic_t) + topic->topic_name_size, pDeserializedInfo->pPublishInfo->pPayload, topic->payload_size);


    adp_topic_publish(ADP_TOPIC_MQTT_INCOMING_TOPIC, topic, event_size, ADP_TOPIC_PRIORITY_HIGH);
*/
}


int32_t mqtt_network_send( NetworkContext_t *pContext, const void* pBuffer, size_t bytes )
{
    uint32_t sent_bytes = adp_ipnet_socket_send(pContext->socket, pBuffer, bytes);
    adp_log_d("MQTT sent %d bytes", sent_bytes);
    return sent_bytes;
}


int32_t mqtt_network_recv( NetworkContext_t *pContext, void* pBuffer, size_t bytes )
{
    uint32_t recv_bytes = adp_ipnet_socket_recv(pContext->socket, pBuffer, bytes);
    if (recv_bytes) {
        adp_log_d("MQTT received %d bytes", recv_bytes);
    }
    return recv_bytes;
}

adp_mqtt_session_t* adp_mqtt_session_alloc(void* socket)
{
    adp_mqtt_session_t *session;

    session = adp_os_malloc(sizeof(adp_mqtt_session_t)); // FIXME remember to free memory in deinit TODO
    ADP_ASSERT(session, "Unable to allocate memory for MQTT session");
    memset(session, 0x00, sizeof(adp_mqtt_session_t));

    // Add to the list
    int i;
    for (i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (s_session_db[i] == NULL) {
            s_session_db[i] = session;
            break;
        }
    }
    if (i == ADP_MQTT_SESSIONS_MAX_NUMBER) {
        adp_log_e("Too many MQTT sessions");
        adp_os_free(session);
        return NULL;
    }

    // Set transport buffer
    MQTTFixedBuffer_t    data_buffer;
    data_buffer.pBuffer = adp_os_malloc(ADP_MQTT_BUFFER_SIZE);
    data_buffer.size    = ADP_MQTT_BUFFER_SIZE;
    ADP_ASSERT(data_buffer.pBuffer, "Unable to allocate memory for MQTT rx/tx buffer");

    // Set transport interface members
    TransportInterface_t      transport_if;
    adp_network_context_t    *network_ctx = adp_os_malloc(sizeof(adp_network_context_t));
    ADP_ASSERT(network_ctx, "Unable to allocate memory for MQTT net context");
    network_ctx->socket = socket;
    transport_if.pNetworkContext = network_ctx;
    transport_if.send = (TransportSend_t)mqtt_network_send;
    transport_if.recv = (TransportRecv_t)mqtt_network_recv;

    MQTTStatus_t core_mqtt_status = MQTT_Init(&session->context,
                                              &transport_if,
                                              adp_os_uptime_ms,
                                              (MQTTEventCallback_t)mqtt_eventCallback,
                                              &data_buffer);

    if( core_mqtt_status != MQTTSuccess ) {
        adp_log_e("MQTT alloc result is %d [%s]", core_mqtt_status, MQTT_Status_strerror(core_mqtt_status));
        adp_mqtt_session_free(session);
        session = NULL;
    }


    return session;
}


void adp_mqtt_session_free(adp_mqtt_session_t* session)
{
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (s_session_db[i] == session) {
            s_session_db[i] = NULL;
            break;
        }
    }

    adp_os_free(session->context.transportInterface.pNetworkContext);
    adp_os_free(session->context.networkBuffer.pBuffer);
    adp_os_free(session->subscribe_info);
    adp_os_free(session);
}


adp_result_t mqtt_do_connect(adp_mqtt_cmd_status_t *result, adp_mqtt_cmd_t* cmd_data)
{
    bool sessionPresent;
    adp_mqtt_cmd_connect_t *connect = (adp_mqtt_cmd_connect_t*)&cmd_data->connect;
    adp_mqtt_session_t     *session = (adp_mqtt_session_t*)cmd_data->session_id;

    if (!connect->client_id) {
        result->status = ADP_RESULT_INVALID_PARAMETER;
        return result->status;
    }
    if (connect->password) {
        session->connect_info.passwordLength     = strlen(connect->password);
    }
    if (connect->username) {
        session->connect_info.userNameLength     = strlen(connect->username);
    }
    session->connect_info.cleanSession           = connect->clean_session;
    session->connect_info.keepAliveSeconds       = connect->keep_alive_seconds;
    session->connect_info.pClientIdentifier      = connect-> client_id;
    session->connect_info.clientIdentifierLength = strlen(connect-> client_id);
    session->connect_info.pPassword              = connect->password;
    session->connect_info.pUserName              = connect->username;

    MQTTStatus_t core_mqtt_status = MQTT_Connect(&session->context,
                                                 &session->connect_info,
                                                 NULL,                    /* No LWT TODO FIXME */
                                                 connect->ack_timeout_ms, /* CONNACK_RECV_TIMEOUT_MS */
                                                 &sessionPresent);
    result->session_id = session;
    result->subcode    = core_mqtt_status;

    if( core_mqtt_status != MQTTSuccess ) {
        adp_log_d("MQTT connect result is %d [%s]", core_mqtt_status, MQTT_Status_strerror(core_mqtt_status));
        result->status = ADP_RESULT_FAILED;
    } else {
        result->status = ADP_RESULT_SUCCESS;
    }

    return result->status;
}


adp_result_t mqtt_do_subscribe(adp_mqtt_cmd_status_t *result, adp_mqtt_cmd_t* cmd_data)
{
    adp_mqtt_cmd_subscribe_t *subscribe = (adp_mqtt_cmd_subscribe_t*)&cmd_data->subscribe;
    adp_mqtt_session_t         *session = (adp_mqtt_session_t*)cmd_data->session_id;

    // Obtain a new packet id for the subscription.
    uint16_t packetId = MQTT_GetPacketId(&session->context);

    session->subscribe_info = adp_os_malloc(sizeof(MQTTSubscribeInfo_t) * subscribe->number_of_filters);
    ADP_ASSERT(session->subscribe_info, "Unable to allocate memory for MQTT filters");
    for (int i = 0; i < subscribe->number_of_filters; i++) {
        ADP_ASSERT(subscribe->filters[i].topic_filter, "Error: MQTT filter name is NULL");
        session->subscribe_info[i].pTopicFilter      = subscribe->filters[i].topic_filter;
        session->subscribe_info[i].topicFilterLength = strlen(subscribe->filters[i].topic_filter);
        session->subscribe_info[i].qos               = subscribe->filters[i].QoS;
    }

    MQTTStatus_t core_mqtt_status= MQTT_Subscribe(&session->context,
                                                  session->subscribe_info,
                                                  subscribe->number_of_filters,
                                                  packetId);

    result->session_id = session;
    result->subcode    = core_mqtt_status;

    if( core_mqtt_status != MQTTSuccess ) {
        adp_log_d("MQTT subscribe result is %d [%s]", core_mqtt_status, MQTT_Status_strerror(core_mqtt_status));
        result->status = ADP_RESULT_FAILED;
    } else {
        result->status = ADP_RESULT_SUCCESS;
    }

    return result->status;
}


int mqtt_socket_recv(uint32_t topic_id, void* data, uint32_t len)
{
    void **socket = data;

    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (!s_session_db[i])
            continue;
        if (s_session_db[i]->context.transportInterface.pNetworkContext->socket == *socket) {
            adp_log_e("MQTT processing sessionId 0x%x", s_session_db[i]);
            MQTT_ProcessLoop(&s_session_db[i]->context, 100);
            break;
        }
    }

    return ADP_RESULT_SUCCESS;
}

int mqtt_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_cmd_t *cmd = (adp_mqtt_cmd_t*)data;
    adp_mqtt_cmd_status_t  result;

    result.command = cmd->command;

    switch (cmd->command) {
    case ADP_MQTT_DO_CONNECT:
        {
            adp_log_d("MQTT - DO_CONNECT");
            mqtt_do_connect(&result, (adp_mqtt_cmd_t*)data);
        }
        break;
    case ADP_MQTT_DO_SUBSCRIBE:
        {
            adp_log_d("MQTT - DO_SUBSCRIBE");
            mqtt_do_subscribe(&result, (adp_mqtt_cmd_t*)data);
        }
        break;
    default:
        adp_log_e("Unknown MQTT cmd #%d", cmd->command);
        result.status = ADP_RESULT_INVALID_PARAMETER;
        break;
    }

    // Notify users on status change
    adp_topic_publish(ADP_TOPIC_MQTT_CMD_STATUS, &result, sizeof(adp_mqtt_cmd_status_t), ADP_TOPIC_PRIORITY_HIGH);

    return ADP_RESULT_SUCCESS;
}


adp_result_t adp_mqtt_initialize(adp_dispatcher_handle_t dispatcher)
{
    memset(s_session_db, 0x00, sizeof(s_session_db));

    // Register topics);
    adp_topic_register(dispatcher, ADP_TOPIC_MQTT_CMD_STATUS,       "MQTT.CmdStatus");
    adp_topic_register(dispatcher, ADP_TOPIC_MQTT_EXECUTE_CMD,      "MQTT.ExecuteCmd");
    adp_topic_register(dispatcher, ADP_TOPIC_MQTT_INCOMING_TOPIC,   "MQTT.IncomingTopic");

    // Subscribe for commands
    adp_topic_subscribe(ADP_TOPIC_MQTT_EXECUTE_CMD,      &mqtt_cmd_handler, "ADP.MQTT.SVC.Executor");
    adp_topic_subscribe(ADP_TOPIC_IPNET_SOCKET_ACTIVITY, &mqtt_socket_recv, "ADP.MQTT.SVC.MonitorIO");

    return ADP_RESULT_SUCCESS;
}
