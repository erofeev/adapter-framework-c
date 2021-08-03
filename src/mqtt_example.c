/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char* client_id;
    int protocol;
    int transport;
} adp_mqtt_client;

typedef struct {
    long host;
    int port;
    int keepalive;
    long bind_address;
} adp_mqtt_netcfg;

int mqtt_example()
{

    return 0;
}
