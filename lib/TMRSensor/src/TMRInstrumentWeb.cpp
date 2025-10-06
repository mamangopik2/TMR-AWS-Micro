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

bool TMRInstrumentWeb::publishBulk(String data, String Timestamp)
{
    if (WiFi.status() != WL_CONNECTED)
        return false;

    WiFiClientSecure client;
    client.setInsecure();

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
        JsonArray updates = entry.createNestedArray("updates");
        JsonObject update = updates.createNestedObject();
        JsonObject value = update.createNestedObject("value");

        JsonObject prop = entry.createNestedObject("properties");
        prop["nitagRetention"] = "DURATION";
        prop["nitagHistoryTTLDays"] = "360";
        prop["nitagMaxHistoryCount"] = "100000";

        update["timestamp"] = Timestamp;
        value["type"] = "DOUBLE";
        value["value"] = scaledValue;
    }

    serializeJson(outputDoc, payload);

    // Serial.println(payload);
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

// ====== Function to send batch ======
int TMRInstrumentWeb::sendBatch(String tagName, TagEntry &tEntry, int start, int count)
{
    if (WiFi.status() != WL_CONNECTED)
        return -1;

    HTTPClient http;
    http.begin(_host + "/nitag/v2/update-current-values");
    http.addHeader("accept", "application/json");
    http.addHeader("x-ni-api-key", _token);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(512);
    JsonArray root = doc.to<JsonArray>();
    JsonObject packet = root.createNestedObject();
    packet["path"] = tagName;
    packet["workspace"] = getWorkspace();

    JsonArray updates = packet.createNestedArray("updates");

    for (int i = 0; i < count; i++)
    {
        UpdateEntry &ue = tEntry.updates[start + i];
        JsonObject upd = updates.createNestedObject();
        JsonObject value = upd.createNestedObject("value");
        value["type"] = "DOUBLE";
        value["value"] = ue.value.toDouble(); // ensure number, not string
        upd["timestamp"] = ue.timestamp;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);

    int httpResponseCode = http.POST(jsonStr);
    Serial.printf("\nPOST -> Tag: %s\n", tagName.c_str());
    Serial.println(jsonStr);
    Serial.printf("Status: %d\n", httpResponseCode);

    if (httpResponseCode > 0)
    {
        String resp = http.getString();
        Serial.println("Response: " + resp);
    }
    else
    {
        Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    doc.clear();
    return httpResponseCode;
}

// ====== Function to parse & send ======
int TMRInstrumentWeb::processCSV(String *csvContent, String timezone)
{
    uint16_t max_retry = 3;
    int failCnt = 0;
    entries.clear(); // reset storage

    // break into lines
    int from = 0;
    while (true)
    {
        int to = csvContent->indexOf('\n', from);
        if (to < 0)
            break;
        String line = csvContent->substring(from, to);
        from = to + 1;

        line.trim();
        if (line.length() == 0)
            continue;

        // split by comma
        int idx1 = line.indexOf(",");
        int idx2 = line.indexOf(",", idx1 + 1);
        int idx3 = line.indexOf(",", idx2 + 1);

        if (idx1 < 0 || idx2 < 0 || idx3 < 0)
            continue;

        String ts = line.substring(0, idx1);
        String tag = line.substring(idx1 + 1, idx2);
        String scaledVal = line.substring(idx2 + 1, idx3);

        UpdateEntry ue = {ts, scaledVal};

        if (entries.find(tag) == entries.end())
        {
            TagEntry t;
            t.path = tag;
            entries[tag] = t;
        }
        entries[tag].updates.push_back(ue);
    }
    *csvContent = ""; // clear the memory
    // ==== batching per tag ====
    for (auto &kv : entries)
    {
        String tagName = kv.first.c_str();
        TagEntry &tEntry = kv.second;

        Serial.printf("\nTag: %s, total updates: %d\n", tagName.c_str(), tEntry.updates.size());

        int repetition = tEntry.updates.size() / MAX_BATCH;
        int remainder = tEntry.updates.size() % MAX_BATCH;
        int index = 0;

        for (int rep = 0; rep < repetition; rep++)
        {
            int retries = 0;
            int statusBatch = -1;

        resendBatch:
            statusBatch = sendBatch(tagName, tEntry, index, MAX_BATCH);

            if ((statusBatch < 200 || statusBatch > 299) && retries < max_retry)
            {
                Serial.printf("Retry batch (attempt %d)\n", retries + 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                retries++;
                goto resendBatch;
            }
            else
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }

            if (statusBatch < 200 || statusBatch > 299)
                failCnt++;

            index += MAX_BATCH;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        if (remainder > 0)
        {
            int retries = 0;
            int statusRem = -1;

        resendRem:
            statusRem = sendBatch(tagName, tEntry, index, remainder);

            if ((statusRem < 200 || statusRem > 299) && retries < max_retry)
            {
                Serial.printf("Retry remainder (attempt %d)\n", retries + 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                retries++;
                goto resendRem;
            }
            else
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }

            if (statusRem < 200 || statusRem > 299)
                failCnt++;

            index += remainder;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

    if (failCnt > 0)
    {
        Serial.printf("\nUpload finished with %d failed batches \n", failCnt);
        return -1;
    }
    else
    {
        Serial.println("\nAll data successfully sent");
        return 1;
    }
}