#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include "MQTTClient.h"
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
 
struct Payload {
    int bay_no;
    int isParked[9];
    time_t timeStamp;
};


#define ADDRESS     "192.168.254.177:1883"
#define CLIENTID    "Pi_publisher"
#define TOPIC       "mqtt/parking/display"
#define DEBUG       0
#define QOS         2
#define TIMEOUT     10000L
#define USERNAME    "pi"
#define PASSWD      "pass"
#define PUB_INTV    4

struct Payload PAYLOAD; 
struct Payload *shm_alloc;
MQTTClient_deliveryToken deliveredtoken;
MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

 
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    #if (DEBUG >= 2)
        printf("Message with token value %d delivery confirmed\n", dt);
    #endif
    deliveredtoken = dt;
    PAYLOAD = *shm_alloc;
    PAYLOAD.timeStamp = time(NULL);
    *shm_alloc = PAYLOAD;
}
 
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    struct Payload* msg;
    int i;
    msg = (struct Payload*)(message->payload);
    #if (DEBUG >= 1)
        printf("\nMessage arrived\n");
        printf("\tTopic:\t%s\n", topicName);
        printf("\tmessage:\n");
        for (i = 0; i < 3; i++) {
            printf("\t%d", msg->bay_no + i);
            printf("\t%d\t%d\t%d\n", msg->isParked[i*2] ? msg->isParked[i*2]: 0, msg->isParked[i*2+1] ? msg->isParked[i*2+1]: 0, msg->isParked[i*2+2] ? msg->isParked[i*2+2] : 0);
        }
    #endif
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}
 
void connlost(void *context, char *cause)
{
    int rc;
    do {
        printf("Reconn...\n");
        rc = MQTTClient_connect(client, &conn_opts);
        sleep(1);
    } while (rc != MQTTCLIENT_SUCCESS);
}
 
int main(int argc, char* argv[])
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc, shmid;
    PAYLOAD.bay_no = 0;

    
    rc = MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        return rc;
    }

    rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        MQTTClient_destroy(&client);
        return rc;
    }
 
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USERNAME;
    conn_opts.password =  PASSWD;

    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        MQTTClient_destroy(&client);
        return rc;
    }

    shmid = shmget(9876, sizeof(struct Payload), 0660);
    shm_alloc = (struct Payload*) shmat(shmid, NULL, 0);

    while (rc == MQTTCLIENT_SUCCESS || rc == MQTTCLIENT_DISCONNECTED) {
        pubmsg.payload = &PAYLOAD;
        pubmsg.payloadlen = sizeof(PAYLOAD);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        deliveredtoken = 0;
        
        rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
        if (rc != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to publish message, return code %d\n", rc);
        }
        else
        {
            #if (DEBUG >= 2)
                printf("\n\nWaiting for publication of %d\n"
                    "on topic %s for client with ClientID: %s\n",
                    rc, TOPIC, CLIENTID);
            #endif
            while (deliveredtoken != token)
            {
                usleep(TIMEOUT);
            }
        }

        #if (DEBUG >= 1)
            rc = MQTTClient_subscribe(client, TOPIC, QOS);
            if (rc != MQTTCLIENT_SUCCESS)
            {
                printf("Failed to Subscribe message, return code %d\n", rc);
            }
        #endif
        sleep(PUB_INTV);
    }

    if ((rc = MQTTClient_disconnect(client, TIMEOUT)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to disconnect, return code %d\n", rc);
        rc = EXIT_FAILURE;
    }
    return rc;
}
