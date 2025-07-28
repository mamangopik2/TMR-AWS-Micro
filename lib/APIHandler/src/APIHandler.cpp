#include "APIHandler.h"

APIHandler::APIHandler()
{
    server = new WebServer(80);
}

void APIHandler::handleHome()
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

void APIHandler::handleScanWifi()
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

String APIHandler::fuseSensor(String existingJson, String newSensorJson)
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

void APIHandler::handleSensorAdd()
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

void APIHandler::handleSensorRead()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server->send(200, "application/json", globalMessage);
    this->sensorUpdated = true;
}

void APIHandler::handleListSensors()
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

void APIHandler::handleUpdateSensors()
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

void APIHandler::handleUpdateSerialCom()
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
void APIHandler::getLogFileList()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");

    // String fileLists = csvLogger->microSD->listDir(csvLogger->microSD->card, "/", 1);

    server->send(200, "application/json", "fileLists");
}

String APIHandler::getContentType(String filename)
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

void APIHandler::handleCloudUpdate()
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
void APIHandler::handleSiteUpdate()
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
void APIHandler::handleTimeUpdate()
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

void APIHandler::handleBeacon()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    beaconUpdatedTime = millis();
    server->send(200, "application/json", "{\"STATUS\":\"OK\"}");
}

void APIHandler::handleRTCSet()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
    RTCJson = server->arg("plain");
    server->send(200, "application/json", "{\"STATUS\":\"OK\"}");
    RTCUpdated = true;
}

unsigned long APIHandler::getBeaconTime()
{
    return this->beaconUpdatedTime;
}

void APIHandler::compile()
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
               std::bind(&APIHandler::handleFileUpload, this));

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
    server->on("/logger-list", [this]()
               { getLogFileList(); });

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
void APIHandler::run()
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

void APIHandler::handleUploadForm()
{
    server->send(200, "text/html", R"rawliteral(
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="upload"><br>
        <input type="submit" value="Upload">
      </form>
    )rawliteral");
}

void APIHandler::handleFileUpload()
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

void APIHandler::handleFileList()
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

void APIHandler::toggleAP()
{
    this->setAPState(!this->getAPState());
}
bool APIHandler::getAPState()
{
    return this->APState;
}
void APIHandler::setAPState(bool state)
{
    this->APState = state;
}

void APIHandler::toggleStaticIP()
{
    this->setStaticIpState(!this->getStaticIpState());
}
bool APIHandler::getStaticIpState()
{
    return this->staticIPState;
}
void APIHandler::setStaticIpState(bool state)
{
    this->staticIPState = state;
}
void APIHandler::writeStaticIpState(bool state)
{
    File file;
    file = SPIFFS.open("/STATIC_IP_STATE.txt", "w");
    if (state == true)
        file.print("true");
    else
        file.print("false");
    file.close();
}