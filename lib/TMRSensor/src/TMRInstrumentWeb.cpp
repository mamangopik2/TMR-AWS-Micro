#include "TMRSensor.h"

bool TMRInstrumentWeb::begin(const char *token)
{
    _token = token;
    return 1;
}

bool TMRInstrumentWeb::publishConfig(String tagName, String data)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = _host + "/nitag/v2/update-current-values";
        String config = *sensorConfiguration;
        Serial.println(url);
        http.begin(url);
        http.addHeader("accept", "application/json");
        http.addHeader("x-ni-api-key", _token);
        http.addHeader("Content-Type", "application/json");

        DynamicJsonDocument doc(512);

        // Root is an array
        JsonArray root = doc.to<JsonArray>();

        // Create entry object inside the array
        JsonObject entry = root.createNestedObject();
        entry["path"] = tagName;
        entry["workspace"] = "Trial";
        entry["properties"] = "{\"sensor_conf\":" + config + "}";

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
        doc.clear();
        // root.clear();
        // entry.clear();
        // updates.clear();
        // update.clear();
        // value.clear();
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

bool TMRInstrumentWeb::parseAndBuildJSON(const String &data, String Timestamp, String &outputPayload)
{
    if (data.indexOf("\"sensors\"") == -1)
    {
        Serial.println("No 'sensors' array found.");
        return false;
    }

    String workspaceId = getWorkspace();
    outputPayload = "[";
    int sensorIndex = 0;

    int sensorsStart = data.indexOf('[');
    int sensorsEnd = data.lastIndexOf(']');
    if (sensorsStart == -1 || sensorsEnd == -1)
        return false;

    String sensorsData = data.substring(sensorsStart + 1, sensorsEnd);
    sensorsData.replace("},{", "}|{"); // temporary delimiter for split

    while (sensorsData.length() > 0)
    {
        int sepIndex = sensorsData.indexOf("|");
        String sensorEntry;

        if (sepIndex != -1)
        {
            sensorEntry = sensorsData.substring(0, sepIndex + 1);
            sensorsData = sensorsData.substring(sepIndex + 2);
        }
        else
        {
            sensorEntry = sensorsData;
            sensorsData = "";
        }

        // Extract tag_name
        int tagStart = sensorEntry.indexOf("\"tag_name\"");
        if (tagStart == -1)
            continue;
        int tagValueStart = sensorEntry.indexOf("\"", tagStart + 11) + 1;
        int tagValueEnd = sensorEntry.indexOf("\"", tagValueStart);
        String tag = sensorEntry.substring(tagValueStart, tagValueEnd);

        // Extract scaled value
        int scaledStart = sensorEntry.indexOf("\"scaled\"");
        if (scaledStart == -1)
            continue;
        int colonIndex = sensorEntry.indexOf(":", scaledStart);
        int commaOrEnd = sensorEntry.indexOf(",", colonIndex + 1);
        if (commaOrEnd == -1)
            commaOrEnd = sensorEntry.indexOf("}", colonIndex + 1);
        String valueStr = sensorEntry.substring(colonIndex + 1, commaOrEnd);
        valueStr.trim();

        Serial.print("Tag: ");
        Serial.print(tag);
        Serial.print(" Value: ");
        Serial.println(valueStr);
        Serial.print("At: ");
        Serial.println(Timestamp);

        // Append to output JSON
        if (sensorIndex++ > 0)
            outputPayload += ",";

        outputPayload += "{";
        outputPayload += "\"path\":\"" + tag + "\",";
        outputPayload += "\"workspace\":\"" + workspaceId + "\",";
        outputPayload += "\"timestamp\":\"" + Timestamp + "\",";
        outputPayload += "\"retention\":\"PERMANENT\",";
        outputPayload += "\"updates\":[{\"value\":{\"type\":\"DOUBLE\",\"value\":" + valueStr + "}}]";
        outputPayload += "}";
    }

    outputPayload += "]";
    return true;
}

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

    String payload;
    // parseAndBuildJSON(data, Timestamp, payload);
    DynamicJsonDocument inputDoc(512);
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

    DynamicJsonDocument outputDoc(512);
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

    serializeJson(outputDoc, payload);

    int httpResponseCode = http.POST(payload);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    inputDoc.clear();
    outputDoc.clear();
    // root.clear();
    // sensors.clear();
    payload = "";

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
            DynamicJsonDocument myJson(512);
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
            myJson.clear();
            // workspaces.clear();
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

String TMRInstrumentWeb::createQuotedText(String text)
{
    return '"' + text + '"';
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