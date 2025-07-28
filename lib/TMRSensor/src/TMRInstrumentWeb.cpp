#include "TMRSensor.h"

bool TMRInstrumentWeb::begin(const char *token)
{
    _token = token;
    return 1;
}

bool TMRInstrumentWeb::publish(String tagName, String data)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = _host + "/nitag/v2/update-current-values";
        Serial.println(url);
        http.begin(url);
        http.addHeader("accept", "application/json");
        http.addHeader("x-ni-api-key", _token);
        http.addHeader("Content-Type", "application/json");

        DynamicJsonDocument doc(1024);

        // Root is an array
        JsonArray root = doc.to<JsonArray>();

        // Create entry object inside the array
        JsonObject entry = root.createNestedObject();
        entry["path"] = tagName;
        entry["workspace"] = getWorkspace();

        // "updates" is an array of objects
        JsonArray updates = entry.createNestedArray("updates");

        // One update object
        JsonObject update = updates.createNestedObject();
        JsonObject value = update.createNestedObject("value");
        value["type"] = "DOUBLE";
        value["value"] = data.toDouble(); // or random(10, 31)

        String jsonString;
        serializeJson(doc, jsonString);
        Serial.println("Sending payload:");
        Serial.println(jsonString);

        // Send the payload with HTTPClient
        int httpResponseCode = http.POST(jsonString);
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode > 0)
        {
            String response = http.getString();
            Serial.println(response);
            return 1;
        }
        else
        {
            Serial.println("Error on sending POST");
            return 0;
        }

        http.end();
    }
    else
    {
        return 0;
    }
}

// bool TMRInstrumentWeb::publishBulk(String data, String Timestamp)
// {
//     if (WiFi.status() != WL_CONNECTED)
//         return false;

//     HTTPClient http;
//     String url = _host + "/nitag/v2/update-current-values";
//     http.begin(url);
//     http.addHeader("accept", "application/json");
//     http.addHeader("x-ni-api-key", _token);
//     http.addHeader("Content-Type", "application/json");

//     DynamicJsonDocument inputDoc(8192);
//     DeserializationError err = deserializeJson(inputDoc, data);
//     if (err)
//     {
//         Serial.print("JSON parse error: ");
//         Serial.println(err.c_str());
//         http.end();
//         http.~HTTPClient();
//         return false;
//     }

//     JsonArray sensors = inputDoc["sensors"];
//     if (!sensors)
//     {
//         http.end();
//         http.~HTTPClient();
//         return false;
//     }

//     DynamicJsonDocument outputDoc(8192); // final payload
//     JsonArray root = outputDoc.to<JsonArray>();
//     String workspaceId = getWorkspace();

//     for (JsonObject sensor : sensors)
//     {
//         const char *tag = sensor["tag_name"];
//         float scaledValue = sensor["value"]["scaled"];

//         Serial.print("Tag:");
//         Serial.print(tag);
//         Serial.print(" Value:");
//         Serial.println(scaledValue);
//         Serial.print("At:");
//         Serial.println(Timestamp);

//         JsonObject entry = root.createNestedObject();
//         entry["path"] = tag;
//         entry["workspace"] = workspaceId;
//         entry["timestamp"] = Timestamp;

//         JsonArray updates = entry.createNestedArray("updates");
//         JsonObject update = updates.createNestedObject();
//         JsonObject value = update.createNestedObject("value");
//         value["type"] = "DOUBLE";
//         value["value"] = scaledValue;
//     }

//     String payload;
//     serializeJson(outputDoc, payload);

//     int httpResponseCode = http.POST(payload);
//     Serial.print("HTTP Response code: ");
//     Serial.println(httpResponseCode);

//     if (httpResponseCode > 0)
//     {
//         String response = http.getString();
//         Serial.println(response);
//         http.end();
//         fail_counter = 0;
//         http.~HTTPClient();
//         return true;
//     }
//     else
//     {
//         Serial.println("Error on sending POST");
//         http.end();
//         fail_counter++;
//         http.~HTTPClient();
//         return false;
//     }
// }

bool TMRInstrumentWeb::publishBulk(String data, String Timestamp)
{
    if (WiFi.status() != WL_CONNECTED)
        return false;

    WiFiClientSecure client;
    client.setInsecure();
    // client.setBufferSizes(512, 512);

    HTTPClient http;
    String url = _host + "/nitag/v2/update-current-values";

    if (!http.begin(client, url))
    {
        Serial.println("Failed to begin HTTPS connection");
        return false;
    }

    http.addHeader("accept", "application/json");
    http.addHeader("x-ni-api-key", _token);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument inputDoc(8192);
    DeserializationError err = deserializeJson(inputDoc, data);
    if (err)
    {
        Serial.print("JSON parse error: ");
        Serial.println(err.c_str());
        http.end();
        return false;
    }

    JsonArray sensors = inputDoc["sensors"];
    if (!sensors)
    {
        http.end();
        return false;
    }

    DynamicJsonDocument outputDoc(8192);
    JsonArray root = outputDoc.to<JsonArray>();
    String workspaceId = getWorkspace();

    for (JsonObject sensor : sensors)
    {
        const char *tag = sensor["tag_name"];
        float scaledValue = sensor["value"]["scaled"];

        Serial.print("Tag: ");
        Serial.print(tag);
        Serial.print(" Value: ");
        Serial.println(scaledValue);
        Serial.print("At: ");
        Serial.println(Timestamp);

        JsonObject entry = root.createNestedObject();
        entry["path"] = tag;
        entry["workspace"] = workspaceId;
        entry["timestamp"] = Timestamp;
        entry["retention"] = "PERMANENT";

        JsonArray updates = entry.createNestedArray("updates");
        JsonObject update = updates.createNestedObject();
        JsonObject value = update.createNestedObject("value");
        value["type"] = "DOUBLE";
        value["value"] = scaledValue;
    }

    String payload;
    serializeJson(outputDoc, payload);

    int httpResponseCode = http.POST(payload);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println(response);
        http.end();
        fail_counter = 0;
        return true;
    }
    else
    {
        Serial.println("Error on sending POST");
        http.end();
        fail_counter++;
        return false;
    }
}

String TMRInstrumentWeb::getWorkspace()
{
    return this->_workspace;
}
void TMRInstrumentWeb::setWorkspace(String id)
{
    this->_workspace = id;
}
bool TMRInstrumentWeb::reqWorkSpace()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = _host + "/niauth/v1/auth";
        Serial.println(url);
        http.begin(url);
        http.addHeader("accept", "application/json");
        http.addHeader("x-ni-api-key", _token);

        int httpResponseCode = http.GET();
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode > 0)
        {
            String response = http.getString();
            DynamicJsonDocument myJson(2048);
            DeserializationError error = deserializeJson(myJson, response);
            Serial.println();
            JsonArray workspaces = myJson["workspaces"];

            for (JsonObject ws : workspaces)
            {
                const char *name = ws["name"];
                const char *id = ws["id"];
                bool isDefault = ws["default"];
                bool enabled = ws["enabled"];

                Serial.print("Name: ");
                Serial.println(name);
                Serial.print("  ID: ");
                Serial.println(id);
                Serial.print("  Default: ");
                Serial.println(isDefault ? "true" : "false");
                Serial.print("  Enabled: ");
                Serial.println(enabled ? "true" : "false");
                Serial.println();

                if (isDefault == true) // only set with default workspace
                {
                    Serial.print("before:");
                    Serial.println(getWorkspace().length());
                    if (String(id).length() > 1)
                    {
                        this->setWorkspace(id);
                    }
                    Serial.print("after:");
                    Serial.println(getWorkspace().length());
                    break;
                }
            }
            return 1;
        }
        else
        {
            Serial.println("Error on sending POST");
            return 0;
        }

        http.end();
    }
    else
    {
        return 0;
    }
}

bool TMRInstrumentWeb::setHost(const char *host)
{
    _host = host;
    return 1;
}
String TMRInstrumentWeb::getHost()
{
    return _host;
}
bool TMRInstrumentWeb::setPort(uint16_t *port)
{
    _port = *port;
    return 1;
}
uint16_t TMRInstrumentWeb::getPort()
{
    return _port;
}
bool TMRInstrumentWeb::setToken(const char *token)
{
    _token = token;
    return 1;
}
String TMRInstrumentWeb::getToken()
{
    return _token;
}