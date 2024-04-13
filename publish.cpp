#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include "MQTTClient.h"
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

#define ADDRESS "tcp://192.168.0.101:1883"
#define CLIENTID "rpi1"
#define AUTHMETHOD "nadia"
#define AUTHTOKEN "123"
#define TOPIC "ee513/CPUTemp"
#define QOS 1
#define TIMEOUT 10000L


int openI2CDevice(int i2cBus, int devAddress) {
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", i2cBus);
    int file = open(filename, O_RDWR);
    if (file < 0) {
        cerr << "Failed to open the i2c bus\n";
        exit(1);
    }
    if (ioctl(file, I2C_SLAVE, devAddress) < 0) {
        cerr << "Failed to acquire bus access and/or talk to slave.\n";
        exit(1);
    }
    return file;
}

void closeI2CDevice(int file) {
    close(file);
}

void writeByte(int file, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    if (write(file, buffer, 2) != 2) {
        cerr << "Failed to write to the i2c bus.\n";
    }
}

int16_t readRawData(int file, int address, uint8_t addr) {
    uint8_t reg = addr;
    write(file, &reg, 1);
    uint8_t data[2] = {0};
    if (read(file, data, 2) != 2) {
        cerr << "Failed to read from the i2c bus.\n";
    }
    int16_t value = (data[0] << 8) | data[1];
    if (value >= 32768) {
        value -= 65536;
    }
    return value;
}

void configureADXL345(int file) {
    writeByte(file, 0x31, 0x0B); // Full resolution, ±16g
    writeByte(file, 0x2C, 0x08); // 25Hz data rate
    writeByte(file, 0x2D, 0x08); // Normal operation mode
    writeByte(file, 0x2E, 0x80); // Enable DATA_READY interrupt
}

void getXYZ(int file, int address, int &x, int &y, int &z) {
    x = readRawData(file, address, 0x32);
    y = readRawData(file, address, 0x34);
    z = readRawData(file, address, 0x36);
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
    int file = openI2CDevice(1, 0x53); // Assuming ADXL345 is on bus 1 at address 0x53
    configureADXL345(file);

    char str_payload[256];  // Increase buffer size to accommodate larger JSON string
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    int rc;
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        cout << "Failed to connect, return code " << rc << endl;
        return -1;
    }

    int x, y, z;
    getXYZ(file, 0x53, x, y, z); // Read accelerometer data
    // Format the JSON string with CPU temperature and accelerometer data
    sprintf(str_payload, "{\"CPUTemp\": %.2f, \"Accel\":{\"X\": %d, \"Y\": %d, \"Z\": %d}}",
            getCPUTemperature(), x, y, z);
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
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    closeI2CDevice(file);
    return rc;
}
