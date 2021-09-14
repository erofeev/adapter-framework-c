/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdint.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_dispatcher.h"
#include "adp_logging.h"
#include "adp_tcpip.h"
#include "app_mqtt.h"

#ifdef ADP_MQTT_AGENT_MODULE_NO_DEBUG
    #undef  adp_log_d
    #undef  adp_log_dd
    #define adp_log_d(...)
    #define adp_log_dd(...)
#endif

adp_mqtt_client_t *s_mqtt_client_db[ADP_MQTT_SESSIONS_MAX_NUMBER];


void do_tcp_connect(adp_mqtt_client_t *user_ctx)
{
    adp_log_d("MQTT client [%s] connecting...", user_ctx->name);
    adp_ipnet_cmd_t ipnet_connect = {
               .user_ctx          = (void*)user_ctx,
               .command           = ADP_IPNET_DO_TCP_CONNECT,
               .connect.port      = user_ctx->port,
               .connect.hostname  = user_ctx->hostname,
               .connect.ip_octet1 = user_ctx->ip_octet1,
               .connect.ip_octet2 = user_ctx->ip_octet2,
               .connect.ip_octet3 = user_ctx->ip_octet3,
               .connect.ip_octet4 = user_ctx->ip_octet4,
    } ;
    adp_topic_publish(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_connect, sizeof(adp_ipnet_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

void do_tcp_shutdown(adp_mqtt_client_t *user_ctx)
{
    adp_log_d("MQTT client [%s] requests TCP shutdown", user_ctx->name);
    adp_ipnet_cmd_t ipnet_shutdown = {
               .user_ctx           = (void*)user_ctx,
               .command            = ADP_IPNET_DO_TCP_SHUTDOWN
    };
    adp_topic_publish(ADP_TOPIC_IPNET_EXECUTE_CMD, &ipnet_shutdown, sizeof(adp_ipnet_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}


void do_mqtt_connect(adp_mqtt_client_t *user_ctx, void *socket, int timeout_ms)
{
    adp_log_d("MQTT client [%s] start negotiation with broker", user_ctx->name);
    adp_mqtt_cmd_t mqtt_connect = {
            .user_ctx                        = (void*)user_ctx,
            .command                         = ADP_MQTT_DO_CONNECT,
            .connect.socket                  = socket,
            .connect.clean_session           = user_ctx->clean_session,
            .connect.keep_alive_seconds      = user_ctx->keep_alive_seconds,
            .connect.client_id               = user_ctx->client_id,
            .connect.username                = user_ctx->username,
            .connect.password                = user_ctx->password,
            .connect.ack_timeout_ms          = user_ctx->ack_timeout_ms,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_connect, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

void do_mqtt_subscribe(adp_mqtt_client_t *user_ctx)
{
    adp_log_d("MQTT client [%s] try subscribing", user_ctx->name);
    adp_mqtt_cmd_t mqtt_subscribe = {
            .user_ctx                        = (void*)user_ctx,
            .command                         = ADP_MQTT_DO_SUBSCRIBE,
            .subscribe.number_of_filters     = user_ctx->number_of_topics,
            .subscribe.filters               = user_ctx->topics,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_subscribe, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

void do_mqtt_disconnect(adp_mqtt_client_t *user_ctx)
{
    adp_log_d("MQTT client [%s] requests MQTT disconnect", user_ctx->name);
    adp_mqtt_cmd_t mqtt_disconnect = {
            .user_ctx                        = (void*)user_ctx,
            .command                         = ADP_MQTT_DO_DISCONNECT,
    };
    adp_topic_publish(ADP_TOPIC_MQTT_EXECUTE_CMD, &mqtt_disconnect, sizeof(adp_mqtt_cmd_t), ADP_TOPIC_PRIORITY_NORMAL);
}

void do_backoff_tcp_connect(adp_os_timer_t timer_obj)
{
    adp_mqtt_client_t *user_ctx = (adp_mqtt_client_t*)adp_os_timer_get_user_ctx(timer_obj);
    adp_os_timer_delete(timer_obj);
    ADP_ASSERT(user_ctx, "Null instead of valid userCtx");
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (s_mqtt_client_db[i] == user_ctx) {
            adp_log_d("MQTT client [%s] scheduled reconnect started for userCtx 0x%x", user_ctx->name, user_ctx);
            do_tcp_connect(user_ctx);
            return;
        }
    }
}

void do_backoff_raise_timer(adp_mqtt_client_t *user_ctx)
{
    // Start one-shot timer
    uint32_t p = (1000 + adp_os_rand()%10000);
    if (!adp_os_timer_start((1000 + adp_os_rand()%10000), 0 /* auto reload */, do_backoff_tcp_connect, user_ctx)) {
        adp_log_e("Unable to raise backoff timer, so start the action right away for userCtx 0x%x", user_ctx);
        do_backoff_tcp_connect(user_ctx);
    } else {
        adp_log_d("MQTT client [%s] reconect will be in %d.%03ds for userCtx 0x%x", user_ctx->name, p/1000, p%1000, user_ctx);
    }
}

// Handling: Socket connection success or not
int net_agent_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_ipnet_cmd_status_t *cmd_status = (adp_ipnet_cmd_status_t*)data;
    adp_mqtt_client_t          *client = (adp_mqtt_client_t*)cmd_status->user_ctx;

    // Filter out sockets which not created by this module
    int found = 0;
    for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
        if (s_mqtt_client_db[i] == cmd_status->user_ctx) {
            found = 1;
            break;
        }
    }
    if (!found)
        return ADP_RESULT_FAILED;

    switch (cmd_status->command) {
    case ADP_IPNET_DO_TCP_CONNECT:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                adp_log_d("MQTT client [%s] TCP connect completed", client->name);
                // Start establishing MQTT session
                do_mqtt_connect(cmd_status->user_ctx, cmd_status->socket, 2000);
            } else {
                adp_log_d("MQTT client [%s] TCP connect failed", client->name);
                // Nothing to disconnect we need to start tcp_connect again after a backoff
                do_backoff_raise_timer(cmd_status->user_ctx);
            }
        }
        break;
    case ADP_IPNET_DO_TCP_SHUTDOWN:
        {
            adp_log_d("MQTT client [%s] TCP shutdown completed, take a backoff and try again", client->name);
            do_backoff_raise_timer(cmd_status->user_ctx);
        }
        break;
    default:
        break;
    }

    return ADP_RESULT_SUCCESS;
}


// Handling: MQTT session
int mqtt_agent_status_handler(uint32_t topic_id, void* data, uint32_t len)
{
    adp_mqtt_cmd_status_t *cmd_status = (adp_mqtt_cmd_status_t*)data;
    adp_mqtt_client_t         *client = (adp_mqtt_client_t*)cmd_status->user_ctx;

    switch (cmd_status->command) {
    case ADP_MQTT_DO_CONNECT:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                adp_log("[%s] MQTT SUCCESFULLY CONNECTED", client->name);
                // Subscribe for topics
                do_mqtt_subscribe(cmd_status->user_ctx);
            } else {
                adp_log_d("MQTT client [%s] TCP connect failed -> shutdown the socket", client->name);
                do_tcp_shutdown(cmd_status->user_ctx);
            }
        }
        break;
    case ADP_MQTT_DO_SUBSCRIBE:
        {
            if (cmd_status->status == ADP_RESULT_SUCCESS) {
                adp_log_d("============================================");
                adp_log_d("[%s] MQTT SUCCESFULLY SUBSCRIBED", client->name);
                adp_log_d("============================================");
            } else {
                // Send DISCONNECT, let's try to clean up everything and try again
                adp_log_d("MQTT client [%s] MQTT failed to subscribe", client->name);
                do_mqtt_disconnect(cmd_status->user_ctx);
            }
        }
        break;
    case ADP_MQTT_DO_DISCONNECT:
        {
            adp_log_d("MQTT client [%s] MQTT terminated", client->name);
            do_tcp_shutdown(cmd_status->user_ctx);
        }
        break;
    default:
        break;
    }

    return ADP_RESULT_SUCCESS;
}


void adp_mqtt_agent_start(adp_mqtt_client_t *client)
{
    static int cnt = 0;

    if (!cnt) {
        // Subscribe for network Up/Down events
        adp_topic_subscribe(ADP_TOPIC_IPNET_CMD_STATUS,    &net_agent_status_handler,  "ADP.MQTT.Agent.IpCmdStatus");
        adp_topic_subscribe(ADP_TOPIC_MQTT_CMD_STATUS,     &mqtt_agent_status_handler, "ADP.MQTT.Agent.MqttCmdStatus");
        memset(s_mqtt_client_db, 0x00, sizeof(s_mqtt_client_db));
    }
    if (cnt == ADP_MQTT_SESSIONS_MAX_NUMBER) {
        adp_log_e("Max number of MQTT clients reached");
        return;
    }
    s_mqtt_client_db[cnt++] = client;
    do_tcp_connect(client);
}


void adp_mqtt_agent_stop(adp_mqtt_client_t *client)
{
    if (!client) {
        for (int i = 0; i < ADP_MQTT_SESSIONS_MAX_NUMBER; i++) {
            if (s_mqtt_client_db[i]) {
                adp_mqtt_client_t *client_i = s_mqtt_client_db[i];
                s_mqtt_client_db[i] = NULL;
                do_mqtt_disconnect(client_i);
                do_tcp_shutdown(client_i);
            }
        }
    } else {
        adp_log_e("MQTT stopping of particular client - is not support yet");
    }
}

