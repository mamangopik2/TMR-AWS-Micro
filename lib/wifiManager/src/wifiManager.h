#if !defined(_wifi_manager_h)
#define _wifi_manager_h

#define DQUOTE "\""
#define NL "\n"
#define CR "\r"
#define defaultSSID "router123"
#define defaultPWD "12345678"
#define defaultAPSSID "TMR AWS"
#define defaultAPPWD "TMR-Instrument"
#define defaultGATEWAY "255.255.255.0"
#define defaultSUBNET "255.255.255.0"
#define defaultSTATICIP "255.255.255.0"
#define defaultBUFFERSIZE "48"
#define defaultCONMODE "1"
#define defaultSERVERIP "192.168.5.5"
#define defaultSERVERPORT "1234"

#include <Arduino.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include "FS.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <ESPmDNS.h>

class wifiManager
{
private:
    File uploadFile;
    bool scanReq = false;
    bool APState = true;
    bool staticIPState = true;

public:
    WebServer *server;
    wifiManager();

    void begin();
    String _AP_SSID, _AP_PWD, _SSID, _PASSWORD, _SUBNET, _GATEWAY, _STATIC_IP, _SERVER_PORT, _SERVER_IP, _BUFFER_SIZE, _CON_MODE;
    String scannedSSID = "";
    String globalMessage;
    String RTCJson;
    String fuseSensor(String existingJson, String newSensorJson);
    bool enableDHCP = false;
    int core = 0;
    int priority = 1;
    unsigned long wifiTimeout = 30000;
    unsigned long beaconUpdatedTime = 1; // if assigned 0 it will not resulting 0 with time multipication
    bool _staticIPMode = true;
    bool automaticAPDisable = true;
    bool sensorUpdated = false;
    bool serialComUpdated = false;
    bool siteUpdated = false;
    bool timeUpdated = false;
    bool cloudUpdated = false;
    bool RTCUpdated = false;

    String getContentType(String filename);

    void compile();
    void run();
    void handleHome();
    void handleScanWifi();
    void handleUploadForm();
    void handleFileUpload();
    void handleFileList();
    void handleSetWireless();
    void handleStaticIp();
    void handleSensorAdd();
    void handleSensorRead();
    void handleListSensors();
    void handleUpdateSensors();
    void handleUpdateSerialCom();
    void handleSiteUpdate();
    void handleTimeUpdate();
    void handleCloudUpdate();
    void handleBeacon();
    void handleRTCSet();
    void toggleAP();
    bool getAPState();
    void setAPState(bool state);
    void toggleStaticIP();
    bool getStaticIpState();
    void setStaticIpState(bool state);
    void writeStaticIpState(bool state);

    unsigned long getBeaconTime();
};

#endif // _wifi_manager_h
