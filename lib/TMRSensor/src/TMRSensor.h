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
#include <SDStorage.h>
#include <Adafruit_ADS1X15.h>
#include <TMRLicenseManager.h>
#include "esp_attr.h"
#include <time.h>
#include <RTClib.h>
#include <map>
#include <vector>

#define MAX_BATCH 30 // maximum buffer sending batch once a time

#if defined USE_SD_LOG
#include <SDStorage.h>
#endif

#define RXD2 16
#define TXD2 17

#define HREG 0
#define IREG 1
#define COIL 2

#define DI1 30
#define DI2 31
#define DI3 32
#define DI4 33

#define PHERI_SLEEP_PIN 32
#define VBAT_SENSE_PIN 34

#define RST_NONE 0
#define RST_MINUTELY 1
#define RST_HOURLY 2
#define RST_DAILY 3
#define RST_MONTHLY 4
#define RST_YEARLY 5

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
    float analogReadData[4] = {0, 0, 0, 0};
    TMRLicenseManager *licenseManager;
    Adafruit_ADS1015 *_ADCInterface1;
    Adafruit_ADS1015 *_ADCInterface2;

    sensorManager();
    String readModbusKF(String EU, String RU, String tagName1, modbusSensor modbus, uint16_t deviceID, uint16_t dataType, uint8_t regType, uint16_t regAddr, uint16_t offsett, bool bigEndian, float kFactor, float ofset);
    String readModbus(String EU, String RU, String tagName2, modbusSensor modbus, uint32_t deviceID, uint8_t dataType, uint16_t regType, uint16_t regAddr, uint8_t offsett, bool bigEndian, float sensitivity, float ofset);
    String readModbus(String EU, String RU, String tagName3, modbusSensor modbus, uint16_t deviceID, uint16_t dataType, uint8_t regType, uint16_t regAddr, uint32_t offsett, bool bigEndian, float readoutMin, float readoutMax, float actualMin, float actualMax);
    void initAnalog(uint8_t address1, uint8_t address2, adsGain_t gain);
    void initAnalog(adsGain_t gain);
    void readRawAnalog();
    String readAnalog_KF(String EU, String RU, String tagName1, uint8_t channel, float kFactor, float ofset);
    String readAnalog_S(String EU, String RU, String tagName2, uint8_t channel, float sensitivity, float ofset);
    String readAnalog_MAP(String EU, String RU, String tagName2, uint8_t channel, float readoutMin, float readoutMax, float actualMin, float actualMax);
    String readDigital(String EU, String RU, String tagName, uint8_t channel);
    String readCoilRegister(String EU, String RU, String tagNameCoil, modbusSensor modbus, uint32_t deviceID, uint16_t regAddr);
};

class TMRInstrumentWeb
{
private:
    struct UpdateEntry
    {
        String timestamp;
        String value;
    };

    struct TagEntry
    {
        String path;
        std::vector<UpdateEntry> updates;
    };

    std::map<String, TagEntry> entries;
    /* data */
public:
    String _host = "",
           _token = "", _username = "", _password = "", _workspace = "";
    uint16_t _port;
    String *sensorConfiguration;

    bool setHost(const char *host);
    String getHost();
    bool setPort(uint16_t *port);
    uint16_t getPort();
    bool setToken(const char *token);
    String getToken();

    uint8_t fail_counter = 0;

    bool begin(const char *token);
    bool publishConfig(String tagName, String data);
    bool publishBulk(String data, String Timestamp);
    String getWorkspace();
    void setWorkspace(String id);
    bool reqWorkSpace();
    String createQuotedText(String text);
    int sendBatch(String tagName, TagEntry &tEntry, int start, int count);
    int processCSV(String *csvContent, String timezone);
};

class configReader
{
private:
public:
    uint16_t modbusHREGS[50][4] = {0};

    /* structure for modbusHREGS Array
    modbusHREGS = {}
    {
        slaveID, regOffset, regAddr, mode
    };
    */

    unsigned long *currentULPUnixTimestamp; // unused
    unsigned long *lastULPUnixTimestamp;    // unused
    unsigned long currentUnixTimestamp = 0; // unused

    HardwareSerial *_modbusPort;
    uint16_t modbusRegistersCount = 0;
    RTC_DS1307 *rtcDevice;
    String _jsonString;
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

    void loadFile();
    void checkUpdate(bool *sensorUpdateFlag);

    void loadSiteInfo();
    void loadTimeInfo();
    void loadCloudInfo();

    String getTimeZone();
    String getTimeSource();
    String getNTPServer();

    void RTCSync();
    void RTCSync(byte *syncFlag);

    float getSensorValue(JsonArray sensors, const char *tag);
    String getSensorsValue(sensorManager &sensManager, modbusSensor &mbInterface);

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

    struct tm timeinfo;
    String getISOTimeNTP();
    String getISOTimeRTC();
    void initRTC();
    unsigned long getUnixTime();

    uint16_t getSecond();
    uint16_t getMinute();
    uint16_t getHour();
    uint16_t getDay();
    uint16_t dayOfWeek();
    uint16_t getMonth();
    uint16_t getYear();
};
class scheduler
{
private:
public:
    SDStorage *microSD;
    uint32_t lastTimeMinuteSend = 0;
    uint32_t lastMinutelyReset = 0;
    uint32_t lastHourlyReset = 0;
    uint32_t lastDailyReset = 0;

    uint16_t (*registers)[4];
    uint16_t *registerCount;
    void manage(String *data, configReader *conf, wifiManager *networkManager, TMRInstrumentWeb *cloud, uint8_t *runUpTimeMinute, unsigned long *clockMinute, uint8_t *logFlag, uint8_t *bufferFlag);
    void deepSleep(unsigned long durationMinute);

    void checkRegReset(configReader *conf, ModbusRTU *_modbusInstance);
    void resetRegisterByMode(ModbusRTU *_modbusInstance, uint8_t mode, uint16_t numOfReg);
    bool sendBuffer(TMRInstrumentWeb *cloud, configReader *conf);
};

#endif // TMR_sensor_h
