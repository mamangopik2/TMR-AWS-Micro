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
    mqtt_client->onMessage([this](String &topic, String &payload)
                           { this->messageReceived(topic, payload); });
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

void TMRemoteMQ::setNetManager(wifiManager *netManager)
{
    networkManager = netManager;
}

void TMRemoteMQ::messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
    if (topic == this->_SN + "/TMRAWS/device/beacon")
    {
        networkManager->beaconUpdatedTime = millis(); // update the beacon time to prevent controller entering sleep mode
    }
    if (topic == this->_SN + "/TMRAWS/device/sensor/update")
    {
        File file;
        file = SPIFFS.open("/sensor.json", "w", true);
        file.print(payload);
        file.close();
        networkManager->sensorUpdated = true; // set update flag to trigger the update
    }
    if (topic == this->_SN + "/TMRAWS/device/cloud/set")
    {
        File file;
        file = SPIFFS.open("/cloud_config.json", "w", true);
        file.print(payload);
        file.close();
        networkManager->cloudUpdated = true; // set update flag to trigger the update
    }
    if (topic == this->_SN + "/TMRAWS/device/time/set")
    {
        File file;
        file = SPIFFS.open("/time_config.json", "w", true);
        file.print(payload);
        file.close();
        networkManager->timeUpdated = true; // set update flag to trigger the update
    }
    if (topic == this->_SN + "/TMRAWS/device/site/set")
    {
        File file;
        file = SPIFFS.open("/site_config.json", "w", true);
        file.print(payload);
        file.close();
        networkManager->siteUpdated = true; // set update flag to trigger the update
    }
    if (topic == this->_SN + "/TMRAWS/device/sensor/add")
    {
        File myJsonFile;
        myJsonFile = SPIFFS.open("/sensor.json", "r");
        String jsonStringFromFile = myJsonFile.readString();
        myJsonFile.close();

        String jsonStringFromClient = payload;
        String newJsonData = networkManager->fuseSensor(jsonStringFromFile, jsonStringFromClient);

        Serial.print("from file");
        Serial.println(jsonStringFromFile);
        Serial.print("from client");
        Serial.println(jsonStringFromClient);
        Serial.print("fused");
        Serial.println(newJsonData);

        myJsonFile = SPIFFS.open("/sensor.json", "w", true);
        myJsonFile.print(newJsonData);
        myJsonFile.close();
        networkManager->sensorUpdated = true; // set update flag to trigger the update
    }
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

void TMRemoteMQ::startThread(uint32_t stackSize, UBaseType_t priority, BaseType_t core)
{
    TaskHandle_t taskHandle;
    xTaskCreatePinnedToCore(
        TMRemoteMQ::run, // Function
        "MyTask",        // Name
        stackSize,       // Stack size in words (not bytes)
        this,            // Task input parameter
        priority,        // Task priority
        &taskHandle,     // Task handle
        core             // Core ID
    );
}

// Static task entry point
void TMRemoteMQ::run(void *parameter)
{
    unsigned long t1 = millis();
    TMRemoteMQ *remote = static_cast<TMRemoteMQ *>(parameter);
    remote->begin("broker.hivemq.com", 1883, "TMR-123");
    while (true)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            if (!remote->mqtt_client->connected())
            {
                remote->connect();
            }
        }
        remote->mqtt_client->loop();

        if (millis() - t1 >= 2000)
        {
            String topic = remote->_SN + "/TMRAWS/device/sensor/data";
            bool status = remote->mqtt_client->publish(topic.c_str(), remote->handledSensorMessage->c_str());
            Serial.print("publish to:");
            Serial.println(topic);

            topic = remote->_SN + "/TMRAWS/device/status";
            status = remote->mqtt_client->publish(topic.c_str(), "ping");
            Serial.print("publish to:");
            Serial.println(topic);

            topic = remote->_SN + "/TMRAWS/device/sensor/list";
            status = remote->mqtt_client->publish(topic.c_str(), remote->configurationManager->_jsonString);
            Serial.print("publish to:");
            Serial.println(topic);
            t1 = millis();
        }
        vTaskDelay(100);
    }
}
