#include "wifiManager.h"

wifiManager::wifiManager()
{
    server = new WebServer(80);
}
void wifiManager::begin(/* args */)
{
    _AP_SSID = "TMR AWS";
    _AP_PWD = "TMR-Instrument";

    WiFi.mode(WIFI_AP_STA);
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    else
    {
        File file = SPIFFS.open("/AP_SSID.txt");
        String buf = "";
        file.close();

        file = SPIFFS.open("/AP_PWD.txt");
        buf = file.readString();
        if (buf.length() >= 8)
            _AP_PWD = buf;
        else
            Serial.println("Failed reading configured AP_PWD");
        file.close();
    }
    String macAddress = String(WiFi.macAddress());
    macAddress.replace(':', '-');
    Serial.println("creating AP");
    Serial.print("SSID:");
    Serial.println(_AP_SSID);
    Serial.print("PWD:");
    Serial.println(_AP_PWD);
    WiFi.softAP(_AP_SSID + macAddress.substring(macAddress.length() - 5), _AP_PWD);

    this->compile();

    scannedSSID = "";
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP_STA);
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n - 1; i++)
    {
        // Serial.println(WiFi.SSID(i));
        scannedSSID += DQUOTE + String(WiFi.SSID(i)) + DQUOTE + NL;
    }
    // Serial.println(WiFi.SSID(n - 1));
    scannedSSID += DQUOTE + String(WiFi.SSID(n - 1)) + DQUOTE;
    Serial.println("======Available SSID=======");
    Serial.println(scannedSSID);
    Serial.println("======Available SSID=======");

    File file;
    Serial.println("saved configuration");
    file = SPIFFS.open("/AP_SSID.txt");
    Serial.println(file.readString());
    file.close();

    file = SPIFFS.open("/AP_PWD.txt");
    Serial.println(file.readString());
    file.close();

    file = SPIFFS.open("/SSID.txt");
    _SSID = file.readString();
    Serial.println(_SSID);
    file.close();

    file = SPIFFS.open("/PASSWORD.txt");
    _PASSWORD = file.readString();
    if (_PASSWORD.length() < 8)
        _PASSWORD = defaultPWD;
    Serial.println(_PASSWORD);
    file.close();

    file = SPIFFS.open("/DEFAULT_STATIC_IP.txt");
    _STATIC_IP = file.readString();
    if (_STATIC_IP.length() < 8)
        _STATIC_IP = defaultSTATICIP;
    Serial.println(_STATIC_IP);
    file.close();

    file = SPIFFS.open("/DEFAULT_GATEWAY.txt");
    _GATEWAY = file.readString();
    if (_GATEWAY.length() < 8)
        _GATEWAY = defaultGATEWAY;
    Serial.println(_GATEWAY);
    file.close();

    file = SPIFFS.open("/DEFAULT_SUBNET.txt");
    _SUBNET = file.readString();
    if (_SUBNET.length() < 8)
        _SUBNET = defaultSUBNET;
    Serial.println(_SUBNET);
    file.close();

    file = SPIFFS.open("/DEFAULT_BUFFER_SIZE.txt");
    _BUFFER_SIZE = file.readString();
    if (_BUFFER_SIZE.length() < 1)
        _BUFFER_SIZE = defaultBUFFERSIZE;
    Serial.println(_BUFFER_SIZE);
    file.close();

    file = SPIFFS.open("/DEFAULT_CON_MODE.txt");
    _CON_MODE = file.readString();
    if (_CON_MODE.length() < 1)
        _CON_MODE = defaultCONMODE;
    Serial.println(_CON_MODE);
    file.close();

    file = SPIFFS.open("/DEFAULT_SERVER_IP.txt");
    _SERVER_IP = file.readString();
    if (_SERVER_IP.length() < 8)
        _SERVER_IP = defaultSERVERIP;
    Serial.println(_SERVER_IP);
    file.close();

    file = SPIFFS.open("/DEFAULT_SERVER_PORT.txt");
    _SERVER_PORT = file.readString();
    if (_SERVER_PORT.length() < 1)
        _SERVER_PORT = defaultSERVERPORT;
    Serial.println(_SERVER_PORT);
    file.close();

    file = SPIFFS.open("/STATIC_IP_STATE.txt");
    if (file.readString() == "true")
    {
        _staticIPMode = true;
    }
    else
    {
        _staticIPMode = false;
    }
    file.close();
}

void wifiManager::handleHome()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    File file = SPIFFS.open("/data_viewer.html", "r");
    if (!file)
    {
        server->send(404, "text/plain", "File Not Found");
        return;
    }
    server->streamFile(file, "text/html");
    file.close();
}

void wifiManager::handleScanWifi()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    scanReq = true;
    // scannedSSID = "";
    // WiFi.disconnect(true);
    // int n = WiFi.scanNetworks();
    // for (int i = 0; i < n - 1; i++)
    // {
    //     // Serial.println(WiFi.SSID(i));
    //     scannedSSID += DQUOTE + String(WiFi.SSID(i)) + DQUOTE + NL;
    // }F
    // // Serial.println(WiFi.SSID(n - 1));
    // scannedSSID += DQUOTE + String(WiFi.SSID(n - 1)) + DQUOTE;
    // Serial.println(scannedSSID);

    String payload;
    payload += "{\"ssid\":[";
    scannedSSID.replace('\n', ',');
    payload += scannedSSID;
    payload += "]}";
    server->send(200, "application/json", payload);
    scanReq = false;
}

String wifiManager::fuseSensor(String existingJson, String newSensorJson)
{
    int sensorStart = existingJson.indexOf("[");
    int sensorEnd = existingJson.lastIndexOf("]");

    if (sensorStart == -1 || sensorEnd == -1 || sensorEnd <= sensorStart)
    {
        return existingJson; // malformed JSON
    }

    String before = existingJson.substring(0, sensorStart + 1);
    String sensorArray = existingJson.substring(sensorStart + 1, sensorEnd);
    String after = existingJson.substring(sensorEnd); // includes "]" and after

    sensorArray.trim();

    String fused;
    if (sensorArray.length() == 0)
    {
        // Empty array
        fused = before + "\n" + newSensorJson + "\n" + after;
    }
    else
    {
        // Existing sensors present, add comma before new
        fused = before + "\n" + sensorArray + ",\n" + newSensorJson + "\n" + after;
    }

    return fused;
}

void wifiManager::handleSensorAdd()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    File myJsonFile;
    myJsonFile = SPIFFS.open("/sensor.json", "r");
    String jsonStringFromFile = myJsonFile.readString();
    myJsonFile.close();

    String jsonStringFromClient = server->arg("plain");
    String newJsonData = fuseSensor(jsonStringFromFile, jsonStringFromClient);

    Serial.print("from file");
    Serial.println(jsonStringFromFile);
    Serial.print("from client");
    Serial.println(jsonStringFromClient);
    Serial.print("fused");
    Serial.println(newJsonData);

    myJsonFile = SPIFFS.open("/sensor.json", "w", true);
    myJsonFile.print(newJsonData);
    myJsonFile.close();

    String payload;
    payload += "{\"status\":\"success\"}";
    server->send(200, "application/json", payload);
    this->sensorUpdated = true;
}

void wifiManager::handleSensorRead()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server->send(200, "application/json", globalMessage);
    this->sensorUpdated = true;
}

void wifiManager::handleListSensors()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");

    File myJsonFile;
    myJsonFile = SPIFFS.open("/sensor.json", "r");
    String jsonStringFromFile = myJsonFile.readString();
    myJsonFile.close();

    server->send(200, "application/json", jsonStringFromFile);
    this->sensorUpdated = true;
}

void wifiManager::handleUpdateSensors()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");

    String jsonStringFromClient = server->arg("plain");
    File myJsonFile = SPIFFS.open("/sensor.json", "w", true);
    myJsonFile.print(jsonStringFromClient);
    myJsonFile.close();
    // Serial.println(jsonStringFromClient);
    this->sensorUpdated = true;

    server->send(200, "application/json", jsonStringFromClient);
    this->sensorUpdated = true;
    jsonStringFromClient = "";
}

void wifiManager::handleUpdateSerialCom()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");

    String jsonStringFromClient = server->arg("plain");
    File myJsonFile = SPIFFS.open("/serial_config.json", "w", true);
    myJsonFile.print(jsonStringFromClient);
    myJsonFile.close();
    // Serial.println(jsonStringFromClient);

    server->send(200, "application/json", jsonStringFromClient);
    this->serialComUpdated = true;
    jsonStringFromClient = "";
}

void wifiManager::handleStaticIp()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    File file;
    if (!server->hasArg("plain"))
    {
        server->send(400, "application/json", "{\"error\":\"No data received\"}");
        return;
    }

    String body = server->arg("plain");
    Serial.println("Received POST data:");
    Serial.println(body);

    // Parse the JSON
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    // Extract data
    String setIP = doc["ip_address"] | defaultSTATICIP;
    String setSubnet = doc["subnet_mask"] | defaultSUBNET;
    String setGateway = doc["gateway"] | defaultGATEWAY;
    Serial.println(setIP);
    Serial.println(setSubnet);
    Serial.println(setGateway);

    file = SPIFFS.open("/DEFAULT_GATEWAY.txt", "w", true);
    file.print(setGateway);
    file.close();

    file = SPIFFS.open("/DEFAULT_STATIC_IP.txt", "w", true);
    file.print(setIP);
    file.close();

    file = SPIFFS.open("/DEFAULT_SUBNET.txt", "w", true);
    file.print(setSubnet);
    file.close();

    // Respond to client
    server->send(200, "application/json", "{\"status\":\"ok\"}");
    ESP.restart();
}

String wifiManager::getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    if (filename.endsWith(".png"))
        return "image/png";
    if (filename.endsWith(".jpg") || filename.endsWith(".jpeg"))
        return "image/jpeg";
    if (filename.endsWith(".gif"))
        return "image/gif";
    if (filename.endsWith(".ico"))
        return "image/x-icon";
    if (filename.endsWith(".json"))
        return "application/json";
    if (filename.endsWith(".pdf"))
        return "application/pdf";
    if (filename.endsWith(".txt"))
        return "text/plain";
    return "application/octet-stream";
}

void wifiManager::handleSetWireless()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    File file;
    if (!server->hasArg("plain"))
    {
        server->send(400, "application/json", "{\"error\":\"No data received\"}");
        return;
    }

    String body = server->arg("plain");
    Serial.println("Received POST data:");
    Serial.println(body);

    // Parse the JSON
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    // Extract data
    String ssid = doc["ssid"] | defaultSSID;
    String password = doc["password"] | defaultPWD;
    String ap_password = doc["ap_password"] | defaultAPPWD;

    if (ap_password.length() < 6)
    {
        Serial.print(ap_password);
        Serial.println(" is too short");
        Serial.println("write default value");
        ap_password = defaultAPPWD;
    }

    if (password.length() < 6)
    {
        Serial.print(password);
        Serial.println(" is too short");
        Serial.println("write default value");
        password = defaultPWD;
    }
    if (ssid.length() < 6)
    {
        Serial.print(ssid);
        Serial.println(" is too short");
        Serial.println("write default value");
        ssid = defaultSSID;
    }
    // Print to Serial
    Serial.println("Parsed wireless settings:");
    Serial.println("STA SSID: " + ssid);
    Serial.println("STA Password: " + password);
    Serial.println("AP Password: " + ap_password);

    file = SPIFFS.open("/AP_PWD.txt", "w", true);
    file.print(ap_password);
    file.close();

    file = SPIFFS.open("/SSID.txt", "w", true);
    file.print(ssid);
    file.close();

    file = SPIFFS.open("/PASSWORD.txt", "w", true);
    file.print(password);
    file.close();

    // Respond to client
    server->send(200, "application/json", "{\"status\":\"ok\"}");
    ESP.restart();
}

void wifiManager::handleCloudUpdate()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");

    String jsonStringFromClient = server->arg("plain");
    File myJsonFile = SPIFFS.open("/cloud_config.json", "w", true);
    myJsonFile.print(jsonStringFromClient);
    myJsonFile.close();
    // Serial.println(jsonStringFromClient);

    server->send(200, "application/json", jsonStringFromClient);
    this->cloudUpdated = true;
    jsonStringFromClient = "";
}
void wifiManager::handleSiteUpdate()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");

    String jsonStringFromClient = server->arg("plain");
    File myJsonFile = SPIFFS.open("/site_config.json", "w", true);
    myJsonFile.print(jsonStringFromClient);
    myJsonFile.close();
    // Serial.println(jsonStringFromClient);

    server->send(200, "application/json", jsonStringFromClient);
    this->siteUpdated = true;
    jsonStringFromClient = "";
}
void wifiManager::handleTimeUpdate()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");

    String jsonStringFromClient = server->arg("plain");
    File myJsonFile = SPIFFS.open("/time_config.json", "w", true);
    myJsonFile.print(jsonStringFromClient);
    myJsonFile.close();
    // Serial.println(jsonStringFromClient);

    server->send(200, "application/json", jsonStringFromClient);
    this->timeUpdated = true;
    jsonStringFromClient = "";
}

void wifiManager::handleBeacon()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    beaconUpdatedTime = millis();
    server->send(200, "application/json", "{\"STATUS\":\"OK\"}");
}

void wifiManager::handleRTCSet()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    RTCJson = server->arg("plain");
    server->send(200, "application/json", "{\"STATUS\":\"OK\"}");
    RTCUpdated = true;
}

unsigned long wifiManager::getBeaconTime()
{
    return this->beaconUpdatedTime;
}

void wifiManager::compile()
{
    Serial.println("Webserver begin");
    server->begin();
    // server->serveStatic("/", SPIFFS, "/"); // allow downloading files directly
    server->on("/", [this]()
               { this->handleHome(); });

    server->on("/scan", [this]()
               { this->handleScanWifi(); });
    server->on("/uploadfile", [this]()
               { handleUploadForm(); });
    server->on("/upload", HTTP_POST, [this]()
               {
                   server->send(200); // Required empty handler
               },
               std::bind(&wifiManager::handleFileUpload, this));

    server->on("/listfile", [this]()
               { handleFileList(); });
    server->on("/setwireless", [this]()
               { handleSetWireless(); });
    server->on("/set-static-ip", [this]()
               { handleStaticIp(); });
    server->on("/push-sensor", [this]()
               { handleSensorAdd(); });
    server->on("/read-sensor", [this]()
               { handleSensorRead(); });
    server->on("/list-sensor", [this]()
               { handleListSensors(); });
    server->on("/update-sensor", [this]()
               { handleUpdateSensors(); });
    server->on("/update-serial-com", [this]()
               { handleUpdateSerialCom(); });

    server->on("/update-cloud", [this]()
               { handleCloudUpdate(); });
    server->on("/update-site", [this]()
               { handleSiteUpdate(); });
    server->on("/update-time", [this]()
               { handleTimeUpdate(); });
    server->on("/beacon", [this]()
               { handleBeacon(); });
    server->on("/rtc-calibration", [this]()
               { handleRTCSet(); });

    server->onNotFound([this]()
                       {
    String path = server->uri();
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        String contentType = getContentType(path);
        server->streamFile(file, contentType);
        file.close();
    } else {
        server->send(404, "text/plain", "File Not Found");
    } });

    server->on("/update", HTTP_POST, [this]()
               {
        server->sendHeader("Connection", "close");
        server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart(); }, [this]()
               {
        HTTPUpload& upload = server->upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("Update Start: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          /* flashing firmware to ESP */
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %u bytes\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        } });
}

unsigned long last_reconnect = 0;
bool lastStaticIPState = true;
bool DNSNotConfigured = true;
void wifiManager::run()
{
    server->handleClient();
    if (millis() - last_reconnect > wifiTimeout)
    {

        if (WiFi.status() == WL_CONNECTED)
        {
            if (automaticAPDisable == true)
            {
                WiFi.softAPdisconnect(true);
            }
            if (DNSNotConfigured == true)
            {
                if (!MDNS.begin("TMR-AWS"))
                { // Set the hostname to "esp32.local"
                    Serial.println("Error setting up MDNS responder!");
                }
                Serial.println("mDNS responder started");
                DNSNotConfigured = false;
            }
        }

        if (WiFi.status() != WL_CONNECTED && scanReq == false)
        {
            Serial.print("connecting to: ");
            Serial.println(_SSID);
            Serial.println("reconnecting WiFi");
            Serial.println(_staticIPMode);
            WiFi.disconnect(true);
            WiFi.begin(_SSID, _PASSWORD);
            if (_staticIPMode == true && this->enableDHCP == false)
            {
                IPAddress IP, GW, SN;
                IP.fromString(_STATIC_IP);
                GW.fromString(_GATEWAY);
                SN.fromString(_SUBNET);
                WiFi.config(IP, GW, SN);
                if (!WiFi.config(IP, GW, SN))
                {
                    Serial.println("STA Failed to configure");
                }
                Serial.println("set static IP");
                Serial.print("IP:");
                Serial.println(IP);
                Serial.print("SN:");
                Serial.println(SN);
                Serial.print("GW:");
                Serial.println(GW);
            }
        }

        last_reconnect = millis();
    }
}

void wifiManager::handleUploadForm()
{
    server->send(200, "text/html", R"rawliteral(
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="upload"><br>
        <input type="submit" value="Upload">
      </form>
    )rawliteral");
}

void wifiManager::handleFileUpload()
{
    HTTPUpload &upload = server->upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = "/" + upload.filename;
        Serial.print("Upload Start: ");
        Serial.println(filename);
        uploadFile = SPIFFS.open(filename, FILE_WRITE);
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (uploadFile)
            uploadFile.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (uploadFile)
        {
            uploadFile.close();
            Serial.print("Upload End, Size: ");
            Serial.println(upload.totalSize);
        }
        server->send(200, "text/plain", "File Uploaded Successfully");
    }
}

void wifiManager::handleFileList()
{
    String html = "<!DOCTYPE html><html><head><title>File Browser</title></head><body>";
    html += "<h2>SPIFFS File List</h2><ul>";

    // Use `SPIFFS.openDir()` for ESP8266 or `SPIFFS.open("/")` + `openNextFile()` for ESP32
    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory())
    {
        server->send(500, "text/plain", "Failed to open directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        String filename = String(file.name());
        Serial.println("Found file: " + filename);

        html += "<li><a href=\"" + filename + "\" download>" + filename + "</a></li>";

        file = root.openNextFile();
    }

    html += "</ul>";
    html += "<br><a href=\"/uploadfile\">Upload More Files</a>";
    html += "</body></html>";

    server->send(200, "text/html", html);
}

void wifiManager::toggleAP()
{
    this->setAPState(!this->getAPState());
}
bool wifiManager::getAPState()
{
    return this->APState;
}
void wifiManager::setAPState(bool state)
{
    this->APState = state;
}

void wifiManager::toggleStaticIP()
{
    this->setStaticIpState(!this->getStaticIpState());
}
bool wifiManager::getStaticIpState()
{
    return this->staticIPState;
}
void wifiManager::setStaticIpState(bool state)
{
    this->staticIPState = state;
}
void wifiManager::writeStaticIpState(bool state)
{
    File file;
    file = SPIFFS.open("/STATIC_IP_STATE.txt", "w");
    if (state == true)
        file.print("true");
    else
        file.print("false");
    file.close();
}