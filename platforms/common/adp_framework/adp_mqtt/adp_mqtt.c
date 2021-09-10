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
#include "adp_tcpip.h"
#include "adp_mqtt.h"


#ifdef ADP_MQTT_MODULE_NO_DEBUG
    #undef  adp_log_d
    #undef  adp_log_dd
    #define adp_log_d(...)
    #define adp_log_dd(...)
#endif


typedef struct NetworkContext {
     void                        *socket;
     void                    *tlsContext;
 } adp_network_context_t;

typedef struct adp_mqtt_session_s {
    void                       *user_ctx;
    uint32_t              ping_timestamp;
    MQTTContext_t                context;
    MQTTConnectInfo_t       connect_info;
    MQTTSubscribeInfo_t  *subscribe_info;
} adp_mqtt_session_t;


adp_mqtt_session_t *s_session_db[ADP_MQTT_SESSIONS_MAX_NUMBER] = {0};
adp_os_mutex_t      s_session_list_mutex = NULL;


static
adp_result_t mqtt_add_session(adp_mqtt_session_t *session)
{
    adp_os_mutex_take(s_session_list_mutex);

    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (!s_session_db[i])
            continue;
        if ( (s_session_db[i]->user_ctx == session->user_ctx) ||
             (s_session_db[i]->context.transportInterface.pNetworkContext->socket == session->context.transportInterface.pNetworkContext->socket) ) {
            adp_os_mutex_give(s_session_list_mutex);
            adp_log_e("Session 0x%x already in the list", session);
            return ADP_RESULT_INVALID_PARAMETER;
        }
    }
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (s_session_db[i] == NULL) {
            s_session_db[i] = session;
            adp_os_mutex_give(s_session_list_mutex);
            adp_log_d("New MQTT session 0x%x added to the list", session);
            return ADP_RESULT_SUCCESS;
        }
    }
    adp_os_mutex_give(s_session_list_mutex);
    adp_log_e("No space for new session 0x%x in the list", session);
    return ADP_RESULT_NO_SPACE_LEFT;
}

static
adp_mqtt_session_t* mqtt_find_session_by_socket(void *socket)
{
    adp_os_mutex_take(s_session_list_mutex);
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (!s_session_db[i])
            continue;
        if (s_session_db[i]->context.transportInterface.pNetworkContext->socket == socket) {
            adp_os_mutex_give(s_session_list_mutex);
            return s_session_db[i];
        }
    }
    adp_os_mutex_give(s_session_list_mutex);
    adp_log_d("Session not in the list for socket 0x%x", socket);
    return NULL;
}

static
adp_mqtt_session_t* mqtt_find_userid_by_socket(void *socket, void **userid)
{
    adp_os_mutex_take(s_session_list_mutex);
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (!s_session_db[i])
            continue;
        if (s_session_db[i]->context.transportInterface.pNetworkContext->socket == socket) {
            *userid = s_session_db[i]->user_ctx;
            adp_os_mutex_give(s_session_list_mutex);
            return s_session_db[i];
        }
    }
    *userid = NULL;
    adp_os_mutex_give(s_session_list_mutex);
    adp_log_d("UserId not in the list for socket 0x%x", socket);
    return NULL;
}

static
adp_mqtt_session_t* mqtt_find_session_by_user_ctx(void *user_ctx)
{
    adp_os_mutex_take(s_session_list_mutex);
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (!s_session_db[i])
            continue;
        if (s_session_db[i]->user_ctx == user_ctx) {
            adp_os_mutex_give(s_session_list_mutex);
            return s_session_db[i];
        }
    }
    adp_os_mutex_give(s_session_list_mutex);
    adp_log_d("Session not in the list for userCtx 0x%x", user_ctx);
    return NULL;
}

static
void mqtt_del_session(adp_mqtt_session_t* session)
{
    adp_os_mutex_take(s_session_list_mutex);
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (s_session_db[i] == session) {
            s_session_db[i] = NULL;
            adp_os_mutex_give(s_session_list_mutex);
            adp_log_d("MQTT session 0x%x is not operational any more", session);
            return;
        }
    }
    adp_os_mutex_give(s_session_list_mutex);
    adp_log_e("Session 0x%x not in the list", session);
    return;
}

static
int32_t mqtt_network_send( NetworkContext_t *pContext, const void* pBuffer, size_t bytes )
{
    if (!mqtt_find_session_by_socket(pContext->socket)) {
        return -1;
    }
    uint32_t sent_bytes = adp_ipnet_socket_send(pContext->socket, pBuffer, bytes);
    adp_log_dd("MQTT sent %d/%d bytes to socket 0x%x", sent_bytes, bytes, pContext->socket);
    return sent_bytes;
}

static
int32_t mqtt_network_recv( NetworkContext_t *pContext, void* pBuffer, size_t bytes )
{
    if (!mqtt_find_session_by_socket(pContext->socket)) {
        return -1; /* Notify MQTT about an error */
    }
    uint32_t recv_bytes = adp_ipnet_socket_recv(pContext->socket, pBuffer, bytes);
    if (recv_bytes) {
        adp_log_dd("MQTT received %d bytes from socket 0x%x", recv_bytes, pContext->socket);
    }
    return recv_bytes;
}

void mqtt_eventCallback(MQTTContext_t *pContext, MQTTPacketInfo_t *pPacketInfo, MQTTDeserializedInfo_t *pDeserializedInfo)
{
    adp_mqtt_session_t * session = mqtt_find_session_by_socket(pContext->transportInterface.pNetworkContext->socket);
    if (!session) {
        adp_log_e("Internal bug: MQTT session unknown");
        return;
    }

    if (pPacketInfo->type == MQTT_PACKET_TYPE_SUBACK) {
        adp_mqtt_cmd_status_t  result;
        result.command    = ADP_MQTT_DO_SUBSCRIBE;
        result.user_ctx   = session->user_ctx;
        result.status     = ADP_RESULT_SUCCESS;
        adp_topic_publish(ADP_TOPIC_MQTT_CMD_STATUS, &result, sizeof(adp_mqtt_cmd_status_t), ADP_TOPIC_PRIORITY_HIGH);
        return;
    }

    if ((pPacketInfo->type & 0xF0U) != MQTT_PACKET_TYPE_PUBLISH) {
        return;
    }

    if (pDeserializedInfo->deserializationResult != MQTTSuccess) {
        adp_log_e("Deserialization error [%s]", MQTT_Status_strerror(pDeserializedInfo->deserializationResult));
        return;
    }

    // Case: MQTT_PACKET_TYPE_PUBLISH
    size_t recv_topic_name_size = pDeserializedInfo->pPublishInfo->topicNameLength;
    size_t recv_payload_size    = pDeserializedInfo->pPublishInfo->payloadLength;
    size_t event_size = sizeof(adp_mqtt_received_topic_t) + recv_topic_name_size + recv_payload_size + 2;

    adp_mqtt_received_topic_t *topic = adp_os_malloc(event_size);
    ADP_ASSERT(topic, "Unable to store incoming data");
    memset(topic, 0x00, event_size);

    topic->session         = session;
    topic->topic_name_size = recv_topic_name_size + 1; // Null terminator
    topic->payload_size    = recv_payload_size;        // Null terminator is present for redundancy but not included in size

    memcpy(&topic->buffer[0],                        pDeserializedInfo->pPublishInfo->pTopicName, recv_topic_name_size);
    memcpy(&topic->buffer[recv_topic_name_size + 1], pDeserializedInfo->pPublishInfo->pPayload,   recv_payload_size);

    adp_topic_publish(ADP_TOPIC_MQTT_INCOMING_TOPIC, topic, event_size, ADP_TOPIC_PRIORITY_HIGH);
    adp_os_free(topic);
}

static
void adp_mqtt_session_free(adp_mqtt_session_t* session)
{
    // Clean up resources
    adp_os_free(session->context.transportInterface.pNetworkContext);
    adp_os_free(session->context.networkBuffer.pBuffer);
    adp_os_free(session->subscribe_info);
    adp_os_free(session);
}

static
adp_mqtt_session_t* adp_mqtt_session_alloc(void* user_ctx, void* socket)
{
    adp_mqtt_session_t *session;

    session = adp_os_malloc(sizeof(adp_mqtt_session_t));
    ADP_ASSERT(session, "Unable to allocate memory for MQTT session");
    memset(session, 0x00, sizeof(adp_mqtt_session_t));

    session->user_ctx = user_ctx;

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
                                              (MQTTEventCallback_t)&mqtt_eventCallback,
                                              &data_buffer);

    if (core_mqtt_status != MQTTSuccess) {
        adp_log_e("MQTT alloc result is %d [%s]", core_mqtt_status, MQTT_Status_strerror(core_mqtt_status));
        adp_mqtt_session_free(session);
        session = NULL;
    } else {
        if (mqtt_add_session(session) != ADP_RESULT_SUCCESS) {
            adp_log_e("MQTT unable to put session 0x%x into the list", session);
            adp_mqtt_session_free(session);
            session = NULL;
        }
    }

    return session;
}

adp_result_t mqtt_do_broker_connect(adp_mqtt_cmd_status_t *result, adp_mqtt_cmd_t* cmd_data)
{
    bool sessionPresent;
    adp_mqtt_cmd_connect_t *connect = (adp_mqtt_cmd_connect_t*)&cmd_data->connect;

    // Allocate new session
    adp_mqtt_session_t *session = adp_mqtt_session_alloc(cmd_data->user_ctx, connect->socket);

    if (!session) {
        result->status = ADP_RESULT_MALLOC_FAILED;
        return result->status;
    }
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
    session->ping_timestamp                      = adp_os_uptime();

    MQTTStatus_t core_mqtt_status = MQTT_Connect(&session->context,
                                                 &session->connect_info,
                                                 NULL,                    /* No LWT support TODO FIXME */
                                                 connect->ack_timeout_ms, /* CONNACK_RECV_TIMEOUT_MS */
                                                 &sessionPresent);
    result->subcode = core_mqtt_status;

    if( core_mqtt_status != MQTTSuccess ) {
        adp_log_d("MQTT connect result is %d [%s]", core_mqtt_status, MQTT_Status_strerror(core_mqtt_status));
        // Deactivate session (the session will become non-operational)
        mqtt_del_session(session);
        adp_mqtt_session_free(session);
        result->status = ADP_RESULT_FAILED;
    } else {
        result->status = ADP_RESULT_SUCCESS;
    }

    return result->status;
}


adp_result_t mqtt_do_subscribe(adp_mqtt_cmd_status_t *result, adp_mqtt_cmd_t* cmd_data)
{
    adp_mqtt_cmd_subscribe_t *subscribe = (adp_mqtt_cmd_subscribe_t*)&cmd_data->subscribe;
    adp_mqtt_session_t         *session = mqtt_find_session_by_user_ctx(cmd_data->user_ctx);

    if (!session) {
        result->status = ADP_RESULT_FAILED;
        return result->status;
    }
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

    MQTTStatus_t core_mqtt_status = MQTT_Subscribe(&session->context,
                                                  session->subscribe_info,
                                                  subscribe->number_of_filters,
                                                  packetId);

    result->subcode = core_mqtt_status;

    if( core_mqtt_status != MQTTSuccess ) {
        adp_log_d("MQTT subscribe result is %d [%s]", core_mqtt_status, MQTT_Status_strerror(core_mqtt_status));
        result->status = ADP_RESULT_FAILED;
    } else {
        result->status = ADP_RESULT_SUCCESS;
    }

    return result->status;
}


adp_result_t mqtt_do_disconnect(adp_mqtt_cmd_status_t *result, adp_mqtt_cmd_t* cmd_data)
{
    adp_mqtt_session_t *session = mqtt_find_session_by_user_ctx(cmd_data->user_ctx);
    if (!session) {
        result->status  = ADP_RESULT_FAILED;
        return result->status;
    }

    // Session is not operational any more
    mqtt_del_session(session);

    MQTTStatus_t core_mqtt_status = MQTT_Disconnect(&session->context);
    result->subcode = core_mqtt_status;

    adp_mqtt_session_free(session);

    if( core_mqtt_status != MQTTSuccess ) {
        adp_log_d("MQTT disconnect result is %d [%s]", core_mqtt_status, MQTT_Status_strerror(core_mqtt_status));
        result->status = ADP_RESULT_FAILED;
    } else {
        result->status = ADP_RESULT_SUCCESS;
    }

    return result->status;
}


int mqtt_socket_monitor_io(uint32_t topic_id, void* data, uint32_t len)
{
    void **socket = data;
    void *user_ctx = NULL;
    adp_mqtt_session_t *session = mqtt_find_userid_by_socket(*socket, &user_ctx);

    if ( (!session) || (!user_ctx) ){
        adp_log_dd("Socket does not belong to MQTT session, topic 0x%x on socket 0x%x", topic_id, *socket);
        return ADP_RESULT_SUCCESS;
    }

    // Notify users on disconnection if SOCKET closed
    if (topic_id == ADP_TOPIC_IPNET_SOCKET_DISCONNECTED) {
        adp_mqtt_cmd_t cmd = {
                .command    = ADP_MQTT_DO_DISCONNECT,
                .user_ctx   = user_ctx,
        };
        adp_log_d("Disconnect received from IPNET");
        adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &cmd, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_HIGH);
        return ADP_RESULT_SUCCESS;
    }

    // RX or TX happened
    if (topic_id == ADP_TOPIC_IPNET_SOCKET_RXTX_ACTIVITY) {
        adp_mqtt_cmd_t cmd = {
                .command    = ADP_MQTT_DO_PROCESS_LOOP,
                .user_ctx   = user_ctx,
        };
        adp_log_d("RX/TX activity received from IPNET");
        adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &cmd, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_HIGH);
    }

    return ADP_RESULT_SUCCESS;
}

int mqtt_cmd_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_cmd_t *cmd = (adp_mqtt_cmd_t*)data;
    adp_mqtt_cmd_status_t  result;

    result.command  = cmd->command;
    result.user_ctx = cmd->user_ctx;
    result.subcode  = 0xADFF; // Internal ADP result code, it means no MQTT subcode provided

    switch (cmd->command) {
    case ADP_MQTT_DO_CONNECT:
        {
            adp_log_d("MQTT - DO_CONNECT");
            mqtt_do_broker_connect(&result, (adp_mqtt_cmd_t*)data);
        }
        break;
    case ADP_MQTT_DO_SUBSCRIBE:
        {
            adp_log_d("MQTT - DO_SUBSCRIBE");
            if (ADP_RESULT_SUCCESS == mqtt_do_subscribe(&result, (adp_mqtt_cmd_t*)data)) {
                // Do not notify user, because ACK should be received and only then we notify the user
                return ADP_RESULT_SUCCESS;
            }
        }
        break;
    case ADP_MQTT_DO_PROCESS_LOOP:
        {
            adp_mqtt_session_t* session = mqtt_find_session_by_user_ctx(cmd->user_ctx);
            if (!session) {
                return ADP_RESULT_SUCCESS;
            }
            if (session->context.connectStatus != MQTTConnected) {
                return ADP_RESULT_SUCCESS;
            }
            adp_log_d("MQTT - DO_PROCESS_LOOP");
            MQTT_ProcessLoop(&session->context, ADP_MQTT_PROCESS_LOOP_TIMEOUT_MS);
            // No status for this command type
            return ADP_RESULT_SUCCESS;
        }
        break;
    case ADP_MQTT_DO_BROKER_PING:
        {
            adp_mqtt_session_t* session = mqtt_find_session_by_user_ctx(cmd->user_ctx);
            if (!session) {
                return ADP_RESULT_SUCCESS;
            }
            if (session->context.connectStatus != MQTTConnected) {
                return ADP_RESULT_SUCCESS;
            }
            adp_log_d("MQTT - DO_BROKER_PING");
            MQTT_Ping(&session->context);
            // No status for this command type
            return ADP_RESULT_SUCCESS;
        }
        break;
    case ADP_MQTT_DO_DISCONNECT:
        {
            adp_log_d("MQTT - DO_DISCONNECT");
            mqtt_do_disconnect(&result, (adp_mqtt_cmd_t*)data);
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

void mqtt_timer_cb(adp_os_timer_t timer_obj)
{
    static uint32_t seconds_counter = 0;
    seconds_counter++;
    adp_os_mutex_take(s_session_list_mutex);
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (!s_session_db[i])
            continue;
        // Check ping settings
        if ((seconds_counter + s_session_db[i]->ping_timestamp) % s_session_db[i]->connect_info.keepAliveSeconds == 0) {
            adp_mqtt_cmd_t cmd;
            cmd.command  = ADP_MQTT_DO_BROKER_PING;
            cmd.user_ctx = s_session_db[i]->user_ctx;
            // Request us to ping the broker
            adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &cmd, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_HIGH);
        }
    }
    adp_os_mutex_give(s_session_list_mutex);
}

adp_result_t adp_mqtt_initialize(adp_dispatcher_handle_t dispatcher)
{
    memset(s_session_db, 0x00, sizeof(s_session_db));
    s_session_list_mutex = adp_os_mutex_create();

    // Published topics
    adp_topic_register(dispatcher, ADP_TOPIC_MQTT_CMD_STATUS,       "MQTT.CmdStatus");
    adp_topic_register(dispatcher, ADP_TOPIC_MQTT_EXECUTE_CMD,      "MQTT.ExecuteCmd");
    adp_topic_register(dispatcher, ADP_TOPIC_MQTT_INCOMING_TOPIC,   "MQTT.IncomingTopic");

    // Subscribe for topics
    adp_topic_subscribe(ADP_TOPIC_MQTT_EXECUTE_CMD,           &mqtt_cmd_handler,       "ADP.MQTT.SVC.Executor");
    adp_topic_subscribe(ADP_TOPIC_IPNET_SOCKET_RXTX_ACTIVITY, &mqtt_socket_monitor_io, "ADP.MQTT.SVC.MonitorIO");
    adp_topic_subscribe(ADP_TOPIC_IPNET_SOCKET_DISCONNECTED,  &mqtt_socket_monitor_io, "ADP.MQTT.SVC.MonitorIO");

    // Start forever timer
    adp_os_timer_t s_mqtt_timer_obj = adp_os_timer_start(1000, 1 /* auto reload */, mqtt_timer_cb);
    UNUSED_VAR(s_mqtt_timer_obj);

    return ADP_RESULT_SUCCESS;
}
