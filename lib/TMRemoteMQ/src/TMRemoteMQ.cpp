#include "TMRemoteMQ.h"
WiFiClient net;
void TMRemoteMQ::begin(String broker, int port, String SN)
{
    mqtt_client = new MQTTClient(8192, 512);
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
    // Serial.print("connecting...");
    while (!mqtt_client->connect(channel.c_str(), "public", "public"))
    {
        // Serial.print(".");
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
    topic = _SN + "/TMRAWS/device/logger/get";
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
        Serial.println(payload);
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
    // if (topic == "TMR-A32/2025/1/0915/ECA8/TMRAWS/device/logger/get")
    if (topic == this->_SN + "/TMRAWS/device/logger/get")
    {
        // Serial.println("streaming CSV file.....");
        // Serial.println(payload);
        this->publishFile(payload);
    }
    if (topic == this->_SN + "/TMRAWS/device/sensor/add")
    {
        File myJsonFile;
        myJsonFile = SPIFFS.open("/sensor.json", "r");
        String jsonStringFromFile = myJsonFile.readString();
        myJsonFile.close();

        String jsonStringFromClient = payload;
        String newJsonData = networkManager->fuseSensor(jsonStringFromFile, jsonStringFromClient);

        // Serial.print("from file");
        // Serial.println(jsonStringFromFile);
        // Serial.print("from client");
        // Serial.println(jsonStringFromClient);
        // Serial.print("fused");
        // Serial.println(newJsonData);

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

void TMRemoteMQ::publishFile(String filepath)
{
    File file;

// Try to open the file from SPIFFS or SD
#ifdef SDSPI
    if (SPIFFS.exists(filepath))
    {
        file = SPIFFS.open(filepath, "r");
    }
    else if (SD.exists(filepath))
    {
        file = SD.open(filepath, "r");
    }
#endif
#ifdef SDMMC
    if (SPIFFS.exists(filepath))
    {
        file = SPIFFS.open(filepath, "r");
    }
    else if (SD_MMC.exists(filepath))
    {
        file = SD_MMC.open(filepath, "r");
    }
#endif
    else
    {
        // Send error message if file not found
        StaticJsonDocument<256> errorDoc;
        errorDoc["file_name"] = filepath;
        errorDoc["batch_total"] = 0;
        errorDoc["batch_number"] = -1;
        errorDoc["data"] = "File Not Found";
        String errorPayload;
        serializeJson(errorDoc, errorPayload);
        errorDoc.clear();
        String topic = this->_SN + "/TMRAWS/data/log/streamCSV";
        uint8_t status = this->mqtt_client->publish(topic.c_str(), errorPayload.c_str());
        // mqtt_client->publish("/TST/TMR/streamCSV", errorPayload.c_str());
        errorPayload = "";
        return;
    }

    const size_t bufferSize = 2 * 1024; // 2 KB buffer
    String buffer = "";
    int batchNumber = 1;
    int batchTotal = 0; // Unknown unless pre-scanned

    // Read and buffer file data
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        if (line.length() < 5)
        {
            break;
        }
        buffer += line + "\n";

        // If buffer reaches the size limit, send it
        if (buffer.length() >= bufferSize)
        {
            StaticJsonDocument<512> doc;
            doc["file_name"] = filepath;
            doc["batch_total"] = batchTotal; // 0 means unknown
            doc["batch_number"] = batchNumber;
            doc["data"] = buffer;

            String jsonPayload;
            serializeJson(doc, jsonPayload);
            doc.clear();
            String topic = this->_SN + "/TMRAWS/data/log/streamCSV";
        sending_packet1:
            uint8_t status = this->mqtt_client->publish(topic.c_str(), jsonPayload.c_str());
            while (status != 1)
            {
                vTaskDelay(100);
                goto sending_packet1;
            }
            jsonPayload = "";
            // Serial.printf("[File Stream] Sent batch %d, size: %d bytes\n", batchNumber, buffer.length());

            buffer = "";
            batchNumber++;
            vTaskDelay(1); // Avoid flooding
        }
    }

    // Send remaining data in buffer
    if (buffer.length() > 0)
    {
        StaticJsonDocument<512> doc;
        doc["file_name"] = filepath;
        doc["batch_total"] = batchTotal;
        doc["batch_number"] = batchNumber;
        doc["data"] = buffer;

        String jsonPayload;
        serializeJson(doc, jsonPayload);
        doc.clear();
        String topic = this->_SN + "/TMRAWS/data/log/streamCSV";
    sending_packet2:
        uint8_t status = this->mqtt_client->publish(topic.c_str(), jsonPayload.c_str());
        while (status != 1)
        {
            vTaskDelay(100);
            goto sending_packet2;
        }
        jsonPayload = "";
        // mqtt_client->publish("/TST/TMR/streamCSV", jsonPayload.c_str());
        // Serial.printf("[File Stream] Sent batch %d, size: %d bytes\n", batchNumber, buffer.length());
    }

    // Send EOF marker
    StaticJsonDocument<512> eofDoc;
    eofDoc["file_name"] = filepath;
    eofDoc["batch_total"] = batchNumber;
    eofDoc["batch_number"] = -1;
    eofDoc["data"] = "EOF";
    String eofPayload;
    serializeJson(eofDoc, eofPayload);
    eofDoc.clear();
    String topic = this->_SN + "/TMRAWS/data/log/streamCSV";
sending_packet3:
    uint8_t status = this->mqtt_client->publish(topic.c_str(), eofPayload.c_str());
    while (status != 1)
    {
        vTaskDelay(100);
        goto sending_packet3;
    }
    eofPayload = "";
    // mqtt_client->publish("/TST/TMR/streamCSV", eofPayload.c_str());
    // Serial.println("[File Stream] Sent EOF");

    file.close();
    batchNumber = 0;
}

void TMRemoteMQ::startThread(uint32_t stackSize, UBaseType_t priority, BaseType_t core)
{
    TaskHandle_t taskHandle;
    xTaskCreatePinnedToCore(
        TMRemoteMQ::run, // Function
        "Remote Task",   // Name
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
    unsigned long t2 = millis();
    unsigned long t3 = millis();
    if (SPIFFS.begin(true) == false)
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    String last_SN = "";
threadStart:
    if (SPIFFS.exists("/SN.txt") == false)
    {
        vTaskDelay(1000);
        File file = SPIFFS.open("/SN.txt", "w", true);
        file.print("TMR-A32" + WiFi.macAddress());
        file.close();
    }
    TMRemoteMQ *remote = static_cast<TMRemoteMQ *>(parameter);
    File file = SPIFFS.open("/SN.txt", "r");
    remote->_SN = file.readString();
    last_SN = remote->_SN;
    file.close();
    Serial.println("Device SN: " + remote->_SN);
    remote->begin(MQTT_BROKER, MQTT_PORT, remote->_SN.c_str());
    while (true)
    {
        if (last_SN != remote->_SN)
        {
            Serial.println("SN changed, restarting...");
            ESP.restart();
        }
        try
        {
            if (millis() - t3 > 5000)
            {
                t3 = millis();
                file = SPIFFS.open("/SN.txt", "r");
                remote->_SN = file.readString();
                file.close();
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                remote->mqtt_client->loop();
                /* code */
                if (!remote->mqtt_client->connected())
                {
                    remote->connect();
                    remote->fail_counter++;
                    remote->fail_counter = 0;
                }
                // check internet connection every 10 after user left configurataion page
                if (millis() - remote->networkManager->getBeaconTime() > (1 * 60 * 1000))
                {
                    if (millis() - t2 >= 10000)
                    {
                        if (!Ping.ping(MQTT_BROKER))
                        {
                            remote->mqtt_client->disconnect();
                            remote->begin(MQTT_BROKER, MQTT_PORT, remote->_SN.c_str());
                        }
                        t2 = millis();
                    }
                }

                if (millis() - t1 >= 1000)
                {
                    String topic;
                    bool status;

                    topic = remote->_SN + "/TMRAWS/device/status";
                    status = remote->mqtt_client->publish(topic.c_str(), "ping");
                    if (status)
                    {
                    }
                    if (millis() - remote->networkManager->beaconUpdatedTime < (1 * 30 * 1000))
                    {
                        topic = remote->_SN + "/TMRAWS/device/sensor/data";
                        status = remote->mqtt_client->publish(topic.c_str(), remote->handledSensorMessage->c_str());
                        if (status)
                        {
                        }
                        topic = remote->_SN + "/TMRAWS/device/sensor/list";
                        status = remote->mqtt_client->publish(topic.c_str(), remote->configurationManager->_jsonString);
                        if (status)
                        {
                        }
                        topic = remote->_SN + "/TMRAWS/device/info";
                        status = remote->mqtt_client->publish(topic.c_str(), *remote->deviceInfo);
                        if (status)
                        {
                        }
                        topic = remote->_SN + "/TMRAWS/data/log/list";
                        status = remote->mqtt_client->publish(topic.c_str(), *remote->logFileList);
                        if (status)
                        {
                        }
                    }

                    t1 = millis();
                }
            }
        }
        catch (const std::exception &e)
        {
            // Serial.println(e.what());
        }

        vTaskDelay(100);
    }
}
