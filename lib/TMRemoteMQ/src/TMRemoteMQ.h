#if !defined(TMRemoteMQ_h)
#define TMRemoteMQ_h
#include "Arduino.h"
#include "MQTT.h"
#include <WiFi.h>
#include "SPIFFS.h"
#include <wifiManager.h>
#include <TMRSensor.h>
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883

class TMRemoteMQ
{
private:
    static void run(void *parameter);

public:
    bool *_flagSensorUpdate, _flagSiteUpdate, _flagTimeUpdate, _flagCloudUpdate;
    bool is_thread_run = false;
    bool thread_enable = false;
    int _port;
    uint8_t fail_counter = 0;
    wifiManager *networkManager;
    String _broker;
    String _SN = "TMR-A32/2025/0/0191/XXXX";
    String *logFileList;
    String *deviceInfo;
    WiFiClient *mqtt_net;
    MQTTClient *mqtt_client;
    String *handledSensorMessage;
    configReader *configurationManager;
    void begin(String broker, int port, String SN);
    bool connect();
    void run();
    void publishFile(String filepath);
    void sendFileBatch(int batchNumber, int batchTotal, String &data);
    void setSensorUpdateFlag(bool *flag);
    void setTimeUpdateFlag(bool *flag);
    void setCloudUpdateFlag(bool *flag);
    void setSiteUpdateFlag(bool *flag);
    void messageReceived(String &topic, String &payload);
    void setNetManager(wifiManager *netkManager);
    void startThread(uint32_t stackSize = 4096,
                     UBaseType_t priority = 1,
                     BaseType_t core = 1);
};

#endif // TMRemoteMQ_h
