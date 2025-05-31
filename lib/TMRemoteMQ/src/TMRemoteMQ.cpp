#include "TMRemoteMQ.h"
WiFiClient net;
void TMRemoteMQ::begin(String broker, int port, String SN)
{
    mqtt_client = new MQTTClient(4096, 4096);
    mqtt_net = new WiFiClient;
    _broker = broker;
    _port = port;
    _SN = SN;
    mqtt_client->begin(broker.c_str(), *mqtt_net);
}
bool TMRemoteMQ::connect()
{
    String channel = String(WiFi.macAddress());
    Serial.print("connecting...");
    while (!mqtt_client->connect(channel.c_str(), "public", "public"))
    {
        Serial.print(".");
        vTaskDelay(1000);
    }
    String topic = _SN + "/TMRAWS/device/beacon";
    mqtt_client->subscribe(topic.c_str());
    topic = _SN + "/TMRAWS/device/cloud/set";
    mqtt_client->subscribe(topic.c_str());
    topic = _SN + "/TMRAWS/device/sensor/update";
    mqtt_client->subscribe(topic.c_str());
    topic = _SN + "/TMRAWS/device/net/set";
    mqtt_client->subscribe(topic.c_str());
    topic = _SN + "/TMRAWS/device/sensor/add";
    mqtt_client->subscribe(topic.c_str());
    topic = _SN + "/TMRAWS/device/site/set";
    mqtt_client->subscribe(topic.c_str());
    topic = _SN + "/TMRAWS/device/time/set";
    mqtt_client->subscribe(topic.c_str());
    return 1;
}

void TMRemoteMQ::setSensorUpdateFlag(bool *flag)
{
    _flagSensorUpdate = flag;
}
void TMRemoteMQ::setTimeUpdateFlag(bool *flag)
{
    _flagTimeUpdate = flag;
}
void TMRemoteMQ::setCloudUpdateFlag(bool *flag)
{
    _flagCloudUpdate = flag;
}
void TMRemoteMQ::setSiteUpdateFlag(bool *flag)
{
    _flagSiteUpdate = flag;
}
