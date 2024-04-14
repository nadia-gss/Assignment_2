//C
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <json-c/json.h>
#include <pigpio.h>


#define ADDRESS     "tcp://192.168.0.102:1883"
#define CLIENTID    "rpi2"
#define AUTHMETHOD  "nadia"
#define AUTHTOKEN   "123"
#define TOPIC       "ee513/CPUTemp"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define PIN 4  // GPIO4 Corresponding BCM pin number

volatile MQTTClient_deliveryToken deliveredtoken;

int initGPIO() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed.\n");
        return -1;
    }
    gpioSetMode(PIN, PI_OUTPUT); 
    return 0; 
}
void setGPIOHigh() {
    gpioWrite(PIN, 1);
}


void setGPIOLow() {
    gpioWrite(PIN, 0); 
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payloadptr;
    struct json_object *parsed_json, *cputemp, *datetime;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = (char*) message->payload;
    parsed_json = json_tokener_parse(payloadptr);
    
    // Temperature data
    cputemp = json_object_object_get(parsed_json, "CPUTemp");
    //Date and time
    datetime = json_object_object_get(parsed_json, "DateTime");

    if (cputemp != NULL && datetime != NULL) {
        double temperature = json_object_get_double(cputemp);
        const char* dateTimeStr = json_object_get_string(datetime);

        printf("CPU Temperature: %.2f°C\n", temperature);
        printf("DateTime: %s\n", dateTimeStr); 

        if (temperature >= 30.0) {
            printf("Warning: CPU temperature is too high!\n");
            printf("LED flashing.\n");
            if (initGPIO() == 0) { 
                for(int i = 0; i <= 5; i++) {
                    setGPIOHigh();       
                    time_sleep(1);      
                    setGPIOLow();
                    time_sleep(1);    
                }   
            }
        }
    } else {
        printf("CPU Temperature data or DateTime not found.\n");
    }

    json_object_put(parsed_json);  
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1; 
}



void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    gpioTerminate();  // Terminate GPIO at the end
    return rc;
}
