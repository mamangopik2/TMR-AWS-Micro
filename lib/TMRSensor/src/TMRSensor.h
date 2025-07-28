#if !defined(TMR_sensor_h)
#define TMR_sensor_h
#include <ModbusRTU.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <wifiManager.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

#if defined USE_SD_LOG
#include <SDStorage.h>
#endif

#define RXD2 27
#define TXD2 26

#define HREG 0
#define IREG 1
#define COIL 2

#define DI1 30
#define DI2 31
#define DI3 32
#define DI4 33

class modbusSensor
{
private:
    /* data */
public:
    modbusSensor();
    ModbusRTU *_modbusInstance;
    HardwareSerial *_modbusPort;
    uint8_t _slaveID = 1;
    void init(HardwareSerial *modbusPort);
    void test();
    uint16_t readUnsignedWord(uint8_t slaveID, uint16_t regType, uint16_t regAddr);
    short readSingleWord(uint8_t slaveID, uint16_t regType, uint16_t regAddr);
    uint32_t readUnsignedInteger(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian);
    int readInteger(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian);
    float readFloat(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian);
    double readDouble(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian);
    bool readCoil(uint8_t slaveID, uint16_t coilAddr);
    void writeCoil(uint8_t slaveID, uint16_t coilAddr, bool value);
};

class sensorManager
{
#define MODBUS_UINT16 1
#define MODBUS_INT16 2
#define MODBUS_UINT32 3
#define MODBUS_INT32 4
#define MODBUS_FLOAT 5
#define MODBUS_DOUBLE 6
private:
    uint32_t registeredSensor = 0;

public:
    Adafruit_ADS1015 *_ADCInterface;
    sensorManager(/* args */);
    String readModbusKF(String EU, String RU, String tagName1, modbusSensor modbus, uint16_t deviceID, uint16_t dataType, uint8_t regType, uint16_t regAddr, uint16_t offsett, bool bigEndian, float kFactor, float ofset);
    String readModbus(String EU, String RU, String tagName2, modbusSensor modbus, uint32_t deviceID, uint8_t dataType, uint16_t regType, uint16_t regAddr, uint8_t offsett, bool bigEndian, float sensitivity, float ofset);
    String readModbus(String EU, String RU, String tagName3, modbusSensor modbus, uint16_t deviceID, uint16_t dataType, uint8_t regType, uint16_t regAddr, uint32_t offsett, bool bigEndian, float readoutMin, float readoutMax, float actualMin, float actualMax);
    void initAnalog(uint8_t address, adsGain_t gain);
    void initAnalog(adsGain_t gain);
    String readAnalog_KF(String EU, String RU, String tagName1, uint8_t channel, float kFactor, float ofset);
    String readAnalog_S(String EU, String RU, String tagName2, uint8_t channel, float sensitivity, float ofset);
    String readAnalog_MAP(String EU, String RU, String tagName2, uint8_t channel, float readoutMin, float readoutMax, float actualMin, float actualMax);
    String readDigital(String EU, String RU, String tagName, uint8_t channel);
    // String readDigital();
};

class TMRInstrumentWeb
{
private:
    /* data */
public:
    String _host = "",
           _token = "", _username = "", _password = "", _workspace = "";
    uint16_t _port;

    bool setHost(const char *host);
    String getHost();
    bool setPort(uint16_t *port);
    uint16_t getPort();
    bool setToken(const char *token);
    String getToken();

    uint8_t fail_counter = 0;

    bool begin(const char *token);
    bool publish(String tagName, String data);
    bool publishBulk(String data, String Timestamp);
    String getWorkspace();
    void setWorkspace(String id);
    bool reqWorkSpace();
};

class configReader
{
private:
public:
    String _jsonString;
    String getSensorsValue(sensorManager &sensManager, modbusSensor &mbInterface);
    void loadFile();
    void checkUpdate(bool *sensorUpdateFlag);
    String _serialComPropertiesJson;
    String _serialBaudrate;
    String _serialMode;
    String _siteInfo;
    String _timeSetup;
    String _cloudSetup;
    String _siteName;
    String _plantName;
    String _deviceName;
    String NTPServer, timezone, timeSource;
    void loadSiteInfo();
    void loadTimeInfo();
    void loadCloudInfo();

    String getTimeZone();
    String getTimeSource();
    String getNTPServer();

    HardwareSerial *_modbusPort;
    int getSerialMode();
    uint32_t getSerialBaud();
    void loadSerialConfigFile();
    void conFigureSerial(HardwareSerial *modbusPort);
    void checkSerialUpdate(bool *serialUpdateFlag, modbusSensor &mbInterface);
    void checkSiteUpdate(bool *siteUpdateFlag);
    void checkTimeUpdate(bool *timeUpdateFlag);
    void checkCloudUpdate(bool *cloudUpdateFlag, TMRInstrumentWeb *cloud);
    void checkRTCUpdate(bool *RTCUpdateFlag, wifiManager *netmanager);
    String getSiteInfo();
    String getCloudHost();
    String getCloudPort();
    String getCloudToken();
    String getCloudInterval();

    bool postSensors(const char *json, TMRInstrumentWeb *cloud);
    struct tm timeinfo;
    String getISOTimeNTP();
    String getISOTimeRTC();
    void initRTC();
};
class scheduler
{
private:
public:
    void manage(const String &data, configReader *conf, wifiManager *networkManager, TMRInstrumentWeb *cloud, uint8_t *runUpTimeMinute, unsigned long *clockMinute, uint8_t *logFlag);
    void deepSleep(unsigned long durationMinute);
};

#endif // TMR_sensor_h
