//C++
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include "MQTTClient.h"
#include "ADXL345.h"
#include <fcntl.h>
#include <stdio.h>
#include <iomanip>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <chrono>
#include <ctime>
#include <array>
#include <cstdint>
#include <bitset>
#define CPU_TEMP "/sys/class/thermal/thermal_zone0/temp"
using namespace std;

#define ADDRESS "tcp://192.168.0.102:1883"
#define CLIENTID "rpi1"
#define AUTHMETHOD "nadia"
#define AUTHTOKEN "123"
#define TOPIC "ee513/CPURoll"
#define QOS 1
#define TIMEOUT 10000L


// Get date and time
string getCurrentDateTime(){
    auto now = chrono::system_clock::now();  
    time_t now_time = chrono::system_clock::to_time_t(now); 
    tm* ptm = localtime(&now_time); 
    ostringstream oss;
    oss << put_time(ptm, "%d-%m-%Y %H:%M:%S"); 
    return oss.str();  
}

float getCPUTemperature() {
    int cpuTemp;
    fstream fs;
    fs.open(CPU_TEMP, fstream::in);
    fs >> cpuTemp;
    fs.close();
    return (((float)cpuTemp) / 1000);
}

int main(int argc, char* argv[]) {
    exploringRPi::ADXL345 accelerometer(1, 0x53); 
    accelerometer.readSensorState();  

    char str_payload[256]; 
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    // Last will
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    will_opts.topicName = TOPIC;
    will_opts.message = "Unexpected disconnection.";
    will_opts.qos = 1;
    will_opts.retained = 0;

    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;
    opts.will = &will_opts;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);


    int rc;
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        cout << "Failed to connect, return code " << rc << endl;
        return -1;
    }

    string datetime = getCurrentDateTime(); // Date and time
    // Get the pitch and roll from the ADXL345 sensor
    float pitch = accelerometer.getPitch();
    float roll = accelerometer.getRoll();

    // Format the JSON string with CPU temperature and accelerometer data
    sprintf(str_payload, "{\"CPUTemp\": %.2f, \"DateTime\":\"%s\",\"Pitch\":%.2f,\"Roll\":%.2f}",
            getCPUTemperature(), datetime.c_str(),pitch,roll);
    pubmsg.payload = str_payload;
    pubmsg.payloadlen = strlen(str_payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    cout << "Waiting for up to " << (int)(TIMEOUT / 1000) <<
         " seconds for publication of " << str_payload <<
         " \non topic " << TOPIC << " for ClientID: " << CLIENTID << endl;
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    cout << "Message with token " << (int)token << " delivered." << endl;
    cout << "Sleeping for 10 seconds. Terminate the program now to trigger the Last Will message.\n";
    sleep(10);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}


