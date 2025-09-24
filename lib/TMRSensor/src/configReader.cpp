#include "TMRSensor.h"

RTC_DS1307 timeRTC;

float configReader::getSensorValue(JsonArray sensors, const char *tag)
{
    for (JsonObject sensor : sensors)
    {
        if (sensor["tag_name"] == tag)
        {
            float value = sensor["value"]["scaled"];
            // //Serial.print("========selected to Math:");
            // //Serial.print(tag);
            // //Serial.println("===========");
            // //Serial.print("========value:");
            // //Serial.print(value);
            // //Serial.println("===========");
            return value;
        }
    }
    return 0;
}

String configReader::getSensorsValue(sensorManager &sensManager, modbusSensor &mbInterface)
{
    uint8_t regType;
    uint16_t byteCounterBuffer = 0;
    String sensorsData[50] = {};
    uint8_t sensorsIndex = 0;
    // ////Serial.println(jsonString);
    DynamicJsonDocument doc(512);

    // Deserialize the JSON string into the document
    DeserializationError error = deserializeJson(doc, _jsonString);
    if (error)
    {
        // //Serial.print("deserializeJson() failed: ");
        // //Serial.println(error.c_str());
    }

    // Access values
    JsonArray sensorArray = doc["sensor"];
    // Iterate through each sensor in the array

    for (JsonObject sensor : sensorArray)
    {
        String sensorData;
        // Basic Info
        const char *tag = sensor["tag_name"];
        const char *phy = sensor["phy"]["channel"];
        // Serial.print("physical:");
        // Serial.println(String(phy));
        if (String(phy) != "MATH")
        {
            // Calibration
            const char *calibration_mode = sensor["calibration"]["calibration_mode"];
            const char *offset_val = sensor["calibration"]["offset"];
            const char *readout_min = sensor["calibration"]["readout_min"];
            const char *readout_max = sensor["calibration"]["readout_max"];
            const char *actual_min = sensor["calibration"]["actual_min"];
            const char *actual_max = sensor["calibration"]["actual_max"];
            const char *k_factor = sensor["calibration"]["k_factor"];
            const char *sensitivity = sensor["calibration"]["sensitivity"];
            // Modbus
            const char *device_id = sensor["modbus"]["modbus_device_id"];
            const char *reg = sensor["modbus"]["modbus_reg"];
            const char *offset = sensor["modbus"]["modbus_offset"];
            const char *reg_type = sensor["modbus"]["modbus_reg_type"];
            const char *dtype = sensor["modbus"]["modbus_dtype"];
            const char *hardware_channel = sensor["modbus"]["hardware_channel"];
            const char *mbBigEndian = sensor["modbus"]["big_endian"];
            const char *resetTime = sensor["modbus"]["modbus_reset_routine"];

            // Digital input
            const char *di_ch = sensor["digital"]["di_ch"];

            // Analog input
            const char *ai_ch = sensor["analog"]["ai_ch"];

            const char *EU = sensor["engineering_unit"];
            const char *RU = sensor["raw_unit"];

            uint16_t dataType;
            // Serial.println(String(dtype));
            if (String(dtype) == "UINT16")
                dataType = MODBUS_UINT16;
            if (String(dtype) == "INT16")
                dataType = MODBUS_INT16;
            if (String(dtype) == "UINT32")
                dataType = MODBUS_UINT32;
            if (String(dtype) == "INT32")
                dataType = MODBUS_INT32;
            if (String(dtype) == "FLOAT32")
                dataType = MODBUS_FLOAT;
            if (String(dtype) == "DOUBLE")
                dataType = MODBUS_DOUBLE;

            // Serial.print("======from config reader: data type: ");
            // Serial.print(dataType);
            // Serial.print("======\n");
            // Serial.print("======from config reader: register type: ");
            // Serial.print(reg_type);
            // Serial.print("======\n");

            if (String(reg_type) == "HREG")
            {
                regType = HREG;
            }
            if (String(reg_type) == "IREG")
            {
                regType = IREG;
            }
            if (String(reg_type) == "COIL")
            {
                regType = COIL;
            }
            if (String(reg_type) == "DISCRETE")
            {
                regType = COIL;
            }

            // //Serial.print("======actual : register: ");
            // //Serial.print(regType);
            // //Serial.print("======\n");

            uint8_t bigEndian;
            if (mbBigEndian == "TRUE")
                bigEndian = true;
            else
                bigEndian = false;

            // debuger.debug(String(phy));
            if (String(phy) == "modbus")
            {
                if (String(calibration_mode) == "1") // kFactor
                {
                    // ////Serial.println("with KF");
                    sensorData = sensManager.readModbusKF(String(EU), String(RU), getSiteInfo() + String(tag), mbInterface, atoi(device_id), dataType, regType, atoi(reg), atoi(offset), bigEndian, atof(k_factor), atof(offset_val));
                }
                else if (String(calibration_mode) == "2") // sensitivity
                {
                    // ////Serial.println("with sensitivity");
                    sensorData = sensManager.readModbus(String(EU), String(RU), getSiteInfo() + String(tag), mbInterface, atoi(device_id), dataType, regType, atoi(reg), atoi(offset), bigEndian, atof(sensitivity), atof(offset_val));
                }
                else if (String(calibration_mode) == "3") // two-points callibration
                {
                    // ////Serial.println("with two-points callibration");
                    sensorData = sensManager.readModbus(String(EU), String(RU), getSiteInfo() + String(tag), mbInterface, atoi(device_id), dataType, regType, atoi(reg), atoi(offset), bigEndian, atof(readout_min), atof(readout_max), atof(actual_min), atof(actual_max));
                }
                else
                {
                    sensorData = sensManager.readModbus(String(EU), String(RU), getSiteInfo() + String(tag), mbInterface, atoi(device_id), dataType, regType, atoi(reg), atoi(offset), bigEndian, atof(k_factor), atof(offset_val));
                }
                if (regType == HREG)
                {
                    modbusHREGS[byteCounterBuffer][0] = atoi(device_id);
                    modbusHREGS[byteCounterBuffer][1] = atoi(offset);
                    modbusHREGS[byteCounterBuffer][2] = atoi(reg);

                    if (String(resetTime) == "MINUTELY")
                        modbusHREGS[byteCounterBuffer][3] = RST_MINUTELY;
                    else if (String(resetTime) == "HOURLY")
                        modbusHREGS[byteCounterBuffer][3] = RST_HOURLY;
                    else if (String(resetTime) == "DAILY")
                        modbusHREGS[byteCounterBuffer][3] = RST_DAILY;
                    else if (String(resetTime) == "MONTHLY")
                        modbusHREGS[byteCounterBuffer][3] = RST_MONTHLY;
                    else if (String(resetTime) == "YEARLY")
                        modbusHREGS[byteCounterBuffer][3] = RST_YEARLY;
                    else
                        modbusHREGS[byteCounterBuffer][3] = RST_NONE; // no reset routine
                    byteCounterBuffer++;
                }
            }
            if (String(phy) == "analog")
            {
                if (String(calibration_mode) == "1") // kFactor
                {
                    try
                    {
                        sensorData = sensManager.readAnalog_KF(String(EU), String(RU), getSiteInfo() + String(tag), atoi(ai_ch), atof(k_factor), atof(offset_val));
                    }
                    catch (const std::exception &e)
                    {
                        sensorData = "{\"tag_name\":\"" + getSiteInfo() + String(tag) + "\","
                                                                                        "\"value\":{\"unscaled\":" +
                                     0 +
                                     ",\"scaled\":" + String(0, 4) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
                    }

                    // ////Serial.println("with KF");
                }
                else if (String(calibration_mode) == "2") // sensitivity
                {
                    try
                    {
                        sensorData = sensManager.readAnalog_S(String(EU), String(RU), getSiteInfo() + String(tag), atoi(ai_ch), atof(sensitivity), atof(offset_val));
                    }
                    catch (const std::exception &e)
                    {
                        sensorData = "{\"tag_name\":\"" + getSiteInfo() + String(tag) + "\","
                                                                                        "\"value\":{\"unscaled\":" +
                                     0 +
                                     ",\"scaled\":" + String(0, 4) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
                    }
                }
                else if (String(calibration_mode) == "3") // two-points callibration
                {
                    // ////Serial.println("with two-points callibration");
                    try
                    {
                        sensorData = sensManager.readAnalog_MAP(String(EU), String(RU), getSiteInfo() + String(tag), atoi(ai_ch), atof(readout_min), atof(readout_max), atof(actual_min), atof(actual_max));
                    }
                    catch (const std::exception &e)
                    {
                        sensorData = "{\"tag_name\":\"" + getSiteInfo() + String(tag) + "\","
                                                                                        "\"value\":{\"unscaled\":" +
                                     0 +
                                     ",\"scaled\":" + String(0, 4) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
                    }
                }
                else
                {
                    try
                    {
                        sensorData = sensManager.readAnalog_KF(String(EU), String(RU), getSiteInfo() + String(tag), atoi(ai_ch), atof(k_factor), atof(offset_val));
                    }
                    catch (const std::exception &e)
                    {
                        sensorData = "{\"tag_name\":\"" + getSiteInfo() + String(tag) + "\","
                                                                                        "\"value\":{\"unscaled\":" +
                                     0 +
                                     ",\"scaled\":" + String(0, 4) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
                    }
                }
            }
            if (String(phy) == "digital")
            {
                sensorData = sensManager.readDigital(String(EU), String(RU), getSiteInfo() + String(tag), atoi(di_ch));
            }
        }
        else
        {

            const char *operand1_type = sensor["operand1"]["type"];
            const char *operand1_val = sensor["operand1"]["value"];
            const char *operand2_type = sensor["operand2"]["type"];
            const char *operand2_val = sensor["operand2"]["value"];
            const char *operation = sensor["operator"];

            sensorData = "{";
            sensorData += "\"tag_name\":\"" + String(tag) + "\",";
            sensorData += "\"phy\":{\"channel\":\"MATH\"},";
            sensorData += "\"operator\":\"" + String(operation) + "\",";
            sensorData += "\"operand1\":{";
            sensorData += "\"type\":\"" + String(operand1_type) + "\",";
            sensorData += "\"value\":\"" + String(operand1_val) + "\"},";
            sensorData += "\"operand2\":{";
            sensorData += "\"type\":\"" + String(operand2_type) + "\",";
            sensorData += "\"value\":\"" + String(operand2_val) + "\"},";
            sensorData += "\"value\":{";
            sensorData += "\"scaled\":0,";
            sensorData += "\"unscaled\":0}";

            sensorData += "}";

            // Serial.println("=========MATH=========");
            // Serial.println(sensorData);
            // Serial.println("=========MATH=========");
        }

        // //Serial.print("============Sensor Data============\n");
        // //Serial.println(sensorData);
        // //Serial.println("===================================\n");
        sensorsData[sensorsIndex] = sensorData;
        sensorsIndex++;
    }
    this->modbusRegistersCount = byteCounterBuffer; // update the modbus registers count

    // Create JSON document
    String jsonOutput;
    jsonOutput += "{\"sensors\":[";
    for (size_t i = 0; i < sensorsIndex; i++)
    {
        if (i > 0)
        {
            jsonOutput += ",";
        }
        jsonOutput += (sensorsData[i]);
    }
    jsonOutput += "]}";
    doc.clear();
    // sensorArray.clear();
    DynamicJsonDocument jsonBuffer(2048);
    DeserializationError err = deserializeJson(jsonBuffer, jsonOutput);

    if (err)
    {
        // Serial.println("JSON parse failed!");
    }

    JsonArray sensors = jsonBuffer["sensors"];

    for (JsonObject sensor : sensors)
    {
        // only process MATH channels
        if (sensor.containsKey("phy") && sensor["phy"]["channel"] == "MATH")
        {
            const char *op = sensor["operator"];
            JsonObject op1 = sensor["operand1"];
            JsonObject op2 = sensor["operand2"];

            float val1 = 0, val2 = 0;

            if (op1["type"] == "sensors")
            {
                val1 = getSensorValue(sensors, op1["value"]);
            }
            else if (op1["type"] == "constant")
            {
                val1 = atof(op1["value"]);
            }

            if (op2["type"] == "sensors")
            {
                val2 = getSensorValue(sensors, op2["value"]);
            }
            else if (op2["type"] == "constant")
            {
                val2 = atof(op2["value"]);
            }

            float result = 0;
            if (strcmp(op, "+") == 0)
                result = val1 + val2;
            else if (strcmp(op, "-") == 0)
                result = val1 - val2;
            else if (strcmp(op, "x") == 0)
                result = val1 * val2;
            else if (strcmp(op, "/") == 0 && val2 != 0)
                result = val1 / val2;

            // Save back to sensor
            sensor["value"]["scaled"] = result;
            sensor["value"]["unscaled"] = result;

            // Serial.printf("Updated %s = %.2f\n", (const char *)sensor["tag_name"], result);
        }
    }
    serializeJson(jsonBuffer, jsonOutput);
    jsonBuffer.clear();

    return jsonOutput;
}
void configReader::loadFile()
{
    SPIFFS.begin();
    vTaskDelay(100);
    File file;
    file = SPIFFS.open("/sensor.json");
    _jsonString = file.readString();
    file.close();
}

void configReader::checkUpdate(bool *sensorUpdateFlag)
{
    if (*sensorUpdateFlag == true) // update sesor configuration if network manager sensor update flag is active
    {
        this->loadFile();
        *sensorUpdateFlag = false;
    }
}

int configReader::getSerialMode()
{
    int mode;
    if (_serialMode == "SERIAL_5N1")
        mode = SERIAL_5N1;
    else if (_serialMode == "SERIAL_6N1")
        mode = SERIAL_6N1;
    else if (_serialMode == "SERIAL_7N1")
        mode = SERIAL_7N1;
    else if (_serialMode == "SERIAL_8N1")
        mode = SERIAL_8N1;
    else if (_serialMode == "SERIAL_5N2")
        mode = SERIAL_5N2;
    else if (_serialMode == "SERIAL_6N2")
        mode = SERIAL_6N2;
    else if (_serialMode == "SERIAL_7N2")
        mode = SERIAL_7N2;
    else if (_serialMode == "SERIAL_8N2")
        mode = SERIAL_8N2;
    else if (_serialMode == "SERIAL_5E1")
        mode = SERIAL_5E1;
    else if (_serialMode == "SERIAL_6E1")
        mode = SERIAL_6E1;
    else if (_serialMode == "SERIAL_7E1")
        mode = SERIAL_7E1;
    else if (_serialMode == "SERIAL_8E1")
        mode = SERIAL_8E1;
    else if (_serialMode == "SERIAL_5E2")
        mode = SERIAL_5E2;
    else if (_serialMode == "SERIAL_6E2")
        mode = SERIAL_6E2;
    else if (_serialMode == "SERIAL_7E2")
        mode = SERIAL_7E2;
    else if (_serialMode == "SERIAL_8E2")
        mode = SERIAL_8E2;
    else if (_serialMode == "SERIAL_5O1")
        mode = SERIAL_5O1;
    else if (_serialMode == "SERIAL_6O1")
        mode = SERIAL_6O1;
    else if (_serialMode == "SERIAL_7O1")
        mode = SERIAL_7O1;
    else if (_serialMode == "SERIAL_8O1")
        mode = SERIAL_8O1;
    else if (_serialMode == "SERIAL_5O2")
        mode = SERIAL_5O2;
    else if (_serialMode == "SERIAL_6O2")
        mode = SERIAL_6O2;
    else if (_serialMode == "SERIAL_7O2")
        mode = SERIAL_7O2;
    else if (_serialMode == "SERIAL_8O2")
        mode = SERIAL_8O2;
    else
    {
        // Default fallback if the input is unknown
        mode = SERIAL_8N1;
    }

    return mode;
}
uint32_t configReader::getSerialBaud()
{
    return _serialBaudrate.toInt();
}

void configReader::loadSerialConfigFile()
{
    File serialConfFile = SPIFFS.open("/serial_config.json");
    this->_serialComPropertiesJson = serialConfFile.readString();
    serialConfFile.close();

    DynamicJsonDocument SerialConfDoc(512);

    // Deserialize the JSON string into the document
    DeserializationError error = deserializeJson(SerialConfDoc, _serialComPropertiesJson);
    if (error)
    {
        // //Serial.print("deserializeJson() failed: ");
        // //Serial.println(error.c_str());
    }
    _serialBaudrate = String((const char *)SerialConfDoc["baudrate"]);
    _serialMode = String((const char *)SerialConfDoc["mode"]);
    // //Serial.println("====serial conf======");
    // //Serial.println(_serialBaudrate);
    // //Serial.println(_serialMode);
    // //Serial.println("====serial conf======");
    SerialConfDoc.clear();
}
void configReader::conFigureSerial(HardwareSerial *modbusPort)
{

    // //Serial.println("Serial 2 pin was assigned successfully");
    Serial2.begin(this->getSerialBaud(), this->getSerialMode(), RXD2, TXD2, false, 300);
    delay(3000);
    // //Serial.println("Serial port was began successfully");
}
void configReader::checkSerialUpdate(bool *serialUpdateFlag, modbusSensor &mbInterface)
{
    if (*serialUpdateFlag == true) // update sesor configuration if network manager sensor update flag is active
    {

        this->loadSerialConfigFile();
        vTaskDelay(2000);
        this->conFigureSerial(_modbusPort);
        vTaskDelay(1000);
        mbInterface.init(&Serial2);
        // //Serial.println("Modbus Port was changed successfully");
        *serialUpdateFlag = false;
    }
}

String configReader::getSiteInfo()
{
    return _siteInfo;
}

void configReader::loadSiteInfo()
{
    File JsonFile = SPIFFS.open("/site_config.json");
    String unparsedJson = JsonFile.readString();
    JsonFile.close();
    DynamicJsonDocument SiteConfDoc(512);
    // Deserialize the JSON string into the document
    DeserializationError error = deserializeJson(SiteConfDoc, unparsedJson);
    String buffer;
    const char *siteName = SiteConfDoc["site_name"];
    const char *plantName = SiteConfDoc["plant_name"];
    const char *deviceName = SiteConfDoc["device_name"];
    _siteName = siteName;
    _deviceName = deviceName;
    _plantName = plantName;
    buffer = String(siteName) + "." + String(plantName) + "." + String(deviceName) + ".";
    _siteInfo = buffer;
    buffer = "";
    SiteConfDoc.clear();
}
void configReader::loadTimeInfo()
{
    File JsonFile = SPIFFS.open("/time_config.json");
    _timeSetup = JsonFile.readString();
    JsonFile.close();
    // //Serial.println(_timeSetup);
}
void configReader::loadCloudInfo()
{
    File JsonFile = SPIFFS.open("/cloud_config.json");
    _cloudSetup = JsonFile.readString();
    JsonFile.close();
}

String configReader::getCloudHost()
{
    DynamicJsonDocument myJson(128);
    DeserializationError error = deserializeJson(myJson, _cloudSetup);
    String data = (const char *)myJson["hostname"];
    myJson.clear();
    return data;
}
String configReader::getCloudPort()
{
    DynamicJsonDocument myJson(128);
    DeserializationError error = deserializeJson(myJson, _cloudSetup);
    String port = (const char *)myJson["port_number"];
    myJson.clear();
    return port;
}
String configReader::getCloudToken()
{
    DynamicJsonDocument myJson(128);
    DeserializationError error = deserializeJson(myJson, _cloudSetup);
    String token = (const char *)myJson["token"];
    myJson.clear();
    return token;
}
String configReader::getCloudInterval()
{
    DynamicJsonDocument myJson(128);
    DeserializationError error = deserializeJson(myJson, _cloudSetup);
    String interval = (const char *)myJson["interval"];
    myJson.clear();
    return interval;
}

void configReader::checkSiteUpdate(bool *siteUpdateFlag)
{
    if (*siteUpdateFlag == true)
    {
        loadSiteInfo();
        *siteUpdateFlag = false;
    }
}

String configReader::getTimeZone()
{
    return timezone;
}
String configReader::getTimeSource()
{
    return timeSource;
}
String configReader::getNTPServer()
{
    return NTPServer;
}

void configReader::checkTimeUpdate(bool *timeUpdateFlag)
{
    if (*timeUpdateFlag == true)
    {
        try
        {
            loadTimeInfo();
            DynamicJsonDocument json(128);
            DeserializationError error = deserializeJson(json, _timeSetup);
            const char *ntpserver = json["ntp_server"];
            const char *tzone = json["time_zone"];
            const char *tsource = json["time_source"];
            NTPServer = ntpserver;
            timezone = tzone;
            timeSource = tsource;
            // //Serial.println(getTimeSource());
            if (getTimeSource() == "NTP")
            {
                configTime((timezone.toFloat() * 3600), 0, NTPServer.c_str());
            }
            else
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                configTime((timezone.toFloat() * 3600), 0, NTPServer.c_str());
                initRTC();
            }
            json.clear();
            *timeUpdateFlag = false;
        }
        catch (const std::exception &e)
        {
            // Serial.println(e.what());
        }
    }
}

void configReader::checkCloudUpdate(bool *cloudUpdateFlag, TMRInstrumentWeb *cloud)
{
    // ////Serial.print("workspace len:");
    // ////Serial.println(cloud->getWorkspace().length());
    if (*cloudUpdateFlag == true || cloud->_workspace.length() < 1)
    {
        loadCloudInfo();
        cloud->begin(this->getCloudToken().c_str());
        cloud->setHost(this->getCloudHost().c_str());
        cloud->reqWorkSpace();
        *cloudUpdateFlag = false;
    }
}

void configReader::initRTC()
{
    if (!timeRTC.begin())
    {
        // //Serial.println("Couldn't find RTC");
    }
    delay(1000);
    if (!timeRTC.isrunning())
    {
        // //Serial.println("RTC is NOT running!");
    }
    rtcDevice = &timeRTC;
}

void configReader::RTCSync()
{

    if (rtcDevice != nullptr)
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("Failed to obtain time");
            return;
        }

        // convert NTP -> DateTime
        int year = timeinfo.tm_year + 1900;
        int month = timeinfo.tm_mon + 1;
        int day = timeinfo.tm_mday;
        int hour = timeinfo.tm_hour;
        int minute = timeinfo.tm_min;
        int second = timeinfo.tm_sec;
        timeRTC.adjust(DateTime(year, month, day, hour, minute, second));
    }
}

uint16_t configReader::getSecond()
{
    if (timeRTC.isrunning())
    {

        DateTime now = timeRTC.now();
        return now.second();
    }
    else
    {
        return 0;
    }
}
uint16_t configReader::getMinute()
{
    if (timeRTC.isrunning())
    {

        DateTime now = timeRTC.now();
        return now.minute();
    }
    else
    {
        return 0;
    }
}
uint16_t configReader::getHour()
{
    if (timeRTC.isrunning())
    {

        DateTime now = timeRTC.now();
        return now.hour();
    }
    else
    {
        return 0;
    }
}
uint16_t configReader::getDay()
{
    if (timeRTC.isrunning())
    {

        DateTime now = timeRTC.now();
        return now.day();
    }
    else
    {
        return 0;
    }
}
uint16_t configReader::dayOfWeek()
{
    if (timeRTC.isrunning())
    {

        DateTime now = timeRTC.now();
        return now.dayOfTheWeek();
    }
    else
    {
        return 0;
    }
}
uint16_t configReader::getMonth()
{
    if (timeRTC.isrunning())
    {

        DateTime now = timeRTC.now();
        return now.month();
    }
    else
    {
        return 0;
    }
}
uint16_t configReader::getYear()
{
    if (timeRTC.isrunning())
    {

        DateTime now = timeRTC.now();
        return now.year();
    }
    else
    {
        return 0;
    }
}

String configReader::getISOTimeNTP()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        bool initTime = true;
        checkTimeUpdate(&initTime);
        return "0000-00-00T00:00:00+00:00"; // Return fallback if time isn't available
    }

    char isoBuffer[25];
    strftime(isoBuffer, sizeof(isoBuffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    String buf = String(isoBuffer);
    buf += "+0" + this->getTimeZone() + ":00";
    return buf;
}
String configReader::getISOTimeRTC()
{
    if (!timeRTC.isrunning())
    {
        initRTC();
        return "0000-00-00T00:00:00+00+00"; // Return fallback if time isn't available
    }
    DateTime now = timeRTC.now();

    char buffer[25];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    String buf = String(buffer);
    buf += "+0" + this->getTimeZone() + ":00";
    return buf;
}

unsigned long configReader::getUnixTime()
{
    if (getTimeSource() == "RTC")
    {
        if (!timeRTC.isrunning())
        {
            initRTC();
            return 0; // Return fallback if RTC isn't running
        }
        DateTime now = timeRTC.now();
        return (now.unixtime());
    }
}
void configReader::checkRTCUpdate(bool *RTCUpdateFlag, wifiManager *netmanager)
{
    if (*RTCUpdateFlag == true)
    {
        DynamicJsonDocument doc(128);
        DeserializationError error = deserializeJson(doc, netmanager->RTCJson);
        // //Serial.println(netmanager->RTCJson);

        int year = doc["year"];
        int month = doc["month"];
        int day = doc["date"];
        int hour = doc["hour"];
        int minute = doc["minute"];
        int second = doc["second"];

        timeRTC.adjust(DateTime(year, month, day, hour, minute, second));
        // //Serial.println("RTC updated successfully.");
        doc.clear();
        *RTCUpdateFlag = false;
    }
}

bool configReader::postSensors(const char *json, TMRInstrumentWeb *cloud)
{
    // return cloud->publishBulk(json);

    // uint32_t cntSuccess = 0;
    // DynamicJsonDocument doc(1024);

    // DeserializationError error = deserializeJson(doc, json);
    // if (error)
    // {
    //     ////Serial.print("Failed to parse JSON: ");
    //     ////Serial.println(error.f_str());
    //     return false;
    // }

    // JsonArray sensors = doc["sensors"];
    // for (JsonObject sensor : sensors)
    // {
    //     const char *tagName = sensor["tag_name"];
    //     float scaled = sensor["value"]["scaled"];

    //     // Call your cloud publishing function
    //     if (cloud->publish(tagName, String(scaled)) == 1)
    //     {
    //         cntSuccess++;
    //     }
    // }
    // if (cntSuccess > 0)
    //     return true;
    // else
    //     return false;
}