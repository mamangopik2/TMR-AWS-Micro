#if !defined(TMRemoteMQ_h)
#define TMRemoteMQ_h
#include "Arduino.h"
#include "MQTT.h"
#include <WiFi.h>
#include "SPIFFS.h"

class TMRemoteMQ
{
private:
    /* data */
public:
    bool *_flagSensorUpdate, _flagSiteUpdate, _flagTimeUpdate, _flagCloudUpdate;
    int _port;
    String _broker;
    String _SN = "AWS123";
    WiFiClient *mqtt_net;
    MQTTClient *mqtt_client;
    void begin(String broker, int port, String SN);
    bool connect();
    void run();
    void setSensorUpdateFlag(bool *flag);
    void setTimeUpdateFlag(bool *flag);
    void setCloudUpdateFlag(bool *flag);
    void setSiteUpdateFlag(bool *flag);
    void messageReceived(String &topic, String &payload);
};

#endif // TMRemoteMQ_h
