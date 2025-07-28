#include <CSVLogger.h>
void CSVLogger::init()
{
    microSD->init();
}
bool CSVLogger::logJsonArray(String filename, String timestampISO, String data)
{

    if (data.length() > 0)
    {
        Serial.println("logging to microSD");
        String buffer_logger = "";
        String dateString = "";

        // buffer_logger = Timestamp + ";" + sensorDataPacket + "\n";

        StaticJsonDocument<8192> doc;
        DeserializationError error = deserializeJson(doc, data);

        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }

        String buffer_row = "";
        // Loop through sensors
        JsonArray sensors = doc["sensors"];
        for (JsonObject sensor : sensors)
        {
            esp_task_wdt_reset();
            String tag = sensor["tag_name"];
            float scaled = sensor["value"]["scaled"];
            float unscaled = sensor["value"]["unscaled"];
            String eng = sensor["eng_unit"];
            String raw = sensor["raw_unit"];
            buffer_logger += (String(timestampISO) + String(",") + (",", tag + "," + String(scaled, 4) + "," + String(unscaled, 4) + "," + eng + "," + raw + "\n"));
        }

        Serial.println(buffer_logger);
        if (!microSD->fileExists(filename))
        {
            microSD->writeFile(microSD->card, filename.c_str(), buffer_logger.c_str());
            return 1;
        }
        else
        {
            microSD->appendFile(microSD->card, filename.c_str(), buffer_logger.c_str());
            return 1;
        }
    }
    else
    {
        return 0;
    }
}
bool CSVLogger::setLogInterval(unsigned long interval)
{
    log_interval = interval;
    return 1;
}
unsigned long CSVLogger::getLogInterval()
{
    return (log_interval);
}
String CSVLogger::readLogFile(String filename)
{
    return "";
}

void CSVLogger::startThread(uint32_t stackSize, UBaseType_t priority, BaseType_t core)
{
    TaskHandle_t taskHandle;
    xTaskCreatePinnedToCore(
        CSVLogger::run, // Function
        "MyTask",       // Name
        stackSize,      // Stack size in words (not bytes)
        this,           // Task input parameter
        priority,       // Task priority
        &taskHandle,    // Task handle
        core            // Core ID
    );
}

// Static task entry point
void CSVLogger::run(void *parameter)
{
    CSVLogger *logger = static_cast<CSVLogger *>(parameter);
    logger->setLogInterval(3000);
    while (true)
    {
        try
        {
            /* code */
            String dateString = "";
            String ISOTimestamp = "";
            Serial.println(logger->configurationManager->getTimeSource());
            if (logger->configurationManager->getTimeSource() == "NTP")
            {
                ISOTimestamp = logger->configurationManager->getISOTimeNTP();
            }
            if (logger->configurationManager->getTimeSource() == "RTC")
            {
                ISOTimestamp = logger->configurationManager->getISOTimeRTC();
            }
            Serial.println(ISOTimestamp);
            if (ISOTimestamp != "0000-00-00T00:00:00Z" && ISOTimestamp != "")
            {
                for (uint32_t i = 0; i < ISOTimestamp.length(); i++)
                {
                    if (ISOTimestamp[i] == 'T')
                    {
                        break;
                    }
                    dateString += ISOTimestamp[i];
                }
                String filename = "/DATA_LOG_" + dateString + ".csv";

                if (logger->handledLoggingMessage->length() > 10 && *logger->loggingFlag == 1)
                {
                    logger->logJsonArray(filename, ISOTimestamp, *logger->handledLoggingMessage);
                    *logger->loggingFlag = 0;
                }
            }
        }
        catch (const std::exception &e)
        {
            Serial.println(e.what());
        }
        vTaskDelay(logger->getLogInterval());
    }
}