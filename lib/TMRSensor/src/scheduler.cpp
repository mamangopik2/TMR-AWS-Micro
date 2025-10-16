#include "TMRSensor.h"
void scheduler::manage(String *data, configReader *conf, wifiManager *networkManager, TMRInstrumentWeb *cloud, uint8_t *runUpTimeMinute, unsigned long *clockMinute, uint8_t *logFlag, uint8_t *bufferFlag)
{
    uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes
    uint32_t RTCMinuteTimeTotal = (conf->getHour() * 60) + conf->getMinute();
    Serial.print("RTC Time(Minute total) = ");
    Serial.println(RTCMinuteTimeTotal);
    bool status;
    if (conf->getCloudInterval().toInt() >= 5) // if the sending interval > 5 minutes, activate sleep mode
    {
        Serial.println("sending interval > 5 minutes");
        if (RTCMinuteTimeTotal % conf->getCloudInterval().toInt() == 0 && lastTimeMinuteSend != RTCMinuteTimeTotal) // if sending time is occur
        {
            lastTimeMinuteSend = RTCMinuteTimeTotal;
            Serial.println("time to sending data");
            Serial.print("sending, Timesource :");
            Serial.println(conf->getTimeSource());
            *logFlag = 1;
            Serial.print("Free Heap: ");
            Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
            Serial.println(" KB");

            if ((freeHeap / 1024) <= 100)
            {
                Serial.println("Not Enough Free Heap");
                *bufferFlag = 1;
                vTaskDelay(10000); // add some delay before going sleep, because the logger have it's own loop cycle
                ESP.restart();
            }
            status = cloud->publishBulk(*data, conf->getISOTimeRTC()); // send sensor data
            Serial.print("Beacon time :");
            Serial.println((millis() - networkManager->getBeaconTime()) / 60 / 1000);
            Serial.print("Timeout after page closed :");
            Serial.println(5);
            if (millis() - networkManager->getBeaconTime() > (5 * 60 * 1000)) // activate sleep after 5 minutes configuration page closed
            {
                if (status) // if success sending sensor data
                {
                    Serial.println("deep sleep");
                    deepSleep(conf->getCloudInterval().toInt() - *runUpTimeMinute); // deep sleep
                }
                else // otherwise save sensor data into buffer file
                {
                    *bufferFlag = 1; // set flag for buffer log
                    *clockMinute = 0;
                    vTaskDelay(10000); // add some delay before going sleep, because the logger have it's own loop cycle
                    Serial.println("deep sleep");
                    deepSleep(conf->getCloudInterval().toInt() - *runUpTimeMinute); // deep sleep
                }
            }
            else // send if user still on the page
            {
                Serial.println("not sleep because the configartion page still opened");
                if (!status)
                {
                    *bufferFlag = 1;
                    *clockMinute = 0;
                }
            }
        }
        else
        {
            Serial.print("Waiting time remain : ");
            Serial.print(conf->getCloudInterval().toInt() - (RTCMinuteTimeTotal % conf->getCloudInterval().toInt()));
            Serial.println(" minutes ahead");
        }
    }
    else
    {
        //  send every 1 minutes without sleep();
        if (*clockMinute >= conf->getCloudInterval().toInt())
        {
            if (conf->getTimeSource() == "NTP")
            {
                *logFlag = 1;
                Serial.print("Free Heap: ");
                Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
                Serial.println(" KB");

                if ((freeHeap / 1024) <= 100)
                {
                    Serial.println("Not Enough Free Heap");
                    *bufferFlag = 1;
                    vTaskDelay(10000);
                    ESP.restart();
                }
                status = cloud->publishBulk(*data, conf->getISOTimeNTP());
                if (!status)
                {
                    *bufferFlag = 1;
                }
            }
            else
            {
                *logFlag = 1;
                Serial.print("Free Heap: ");
                Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
                Serial.println(" KB");

                if ((freeHeap / 1024) <= 100)
                {
                    Serial.println("Not Enough Free Heap");
                    vTaskDelay(10000);
                    ESP.restart();
                }
                status = cloud->publishBulk(*data, conf->getISOTimeRTC());
                if (!status)
                {
                    *bufferFlag = 1;
                }
            }
            *clockMinute = 0; // reset the minute timer after send the data
        }
    }
}

void scheduler::checkRegReset(configReader *conf, ModbusRTU *_modbusInstance)
{
    uint32_t absoluteMinuteTime;
    absoluteMinuteTime = (conf->getHour() * 60) + conf->getMinute();

    if (absoluteMinuteTime % 1440 == 0)
    {
        if (lastDailyReset != absoluteMinuteTime)
        {
            this->resetRegisterByMode(_modbusInstance, RST_DAILY, conf->modbusRegistersCount);
            lastDailyReset = absoluteMinuteTime;
        }
    }
    else if (absoluteMinuteTime % 60 == 0)
    {
        if (lastHourlyReset != absoluteMinuteTime)
        {
            this->resetRegisterByMode(_modbusInstance, RST_HOURLY, conf->modbusRegistersCount);
            lastHourlyReset = absoluteMinuteTime;
        }
    }
    else if (absoluteMinuteTime % 5 == 0)
    {
        if (lastMinutelyReset != absoluteMinuteTime)
        {
            this->resetRegisterByMode(_modbusInstance, RST_MINUTELY, conf->modbusRegistersCount);
            lastMinutelyReset = absoluteMinuteTime;
        }
    }
    else
    {
        NULL;
    }
}

void scheduler::resetRegisterByMode(ModbusRTU *_modbusInstance, uint8_t mode, uint16_t numOfReg)
{
    Serial.print("registerCount:");
    Serial.println(numOfReg);
    for (uint16_t i = 0; i < numOfReg; i++)
    {
        Serial.print("Register Lists: ");
        Serial.print("Slave ID: ");
        Serial.print(registers[i][0]);
        Serial.print(" Reg Offset: ");
        Serial.print(registers[i][1]);
        Serial.print(" Reg Addr: ");
        Serial.print(registers[i][2]);
        Serial.print(" Reset Routine: ");

        if (registers[i][3] == RST_NONE)
        {
            Serial.println("No Reset Routine");
            continue; // skip if no reset routine
        }
        else if (registers[i][3] == RST_MINUTELY)
        {
            Serial.println("Minutely Reset Routine");
        }
        else if (registers[i][3] == RST_HOURLY)
        {
            Serial.println("Hourly Reset Routine");
        }
        else if (registers[i][3] == RST_DAILY)
        {
            Serial.println("Daily Reset Routine");
        }
        else if (registers[i][3] == RST_MONTHLY)
        {
            Serial.println("Monthly Reset Routine");
        }
        else if (registers[i][3] == RST_YEARLY)
        {
            Serial.println("Yearly Reset Routine");
        }

        Serial.print("reset status:");
        Serial.println(mode == registers[i][3]);
        if (mode == registers[i][3])
        {
            Serial.println("Register RESET is RUN");
            uint16_t val = 0;
            _modbusInstance->writeHreg(registers[i][0], registers[i][1] + registers[i][2], &val);
            _modbusInstance->task();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void scheduler::deepSleep(unsigned long durationMinute)
{
    digitalWrite(PHERI_SLEEP_PIN, LOW);
    unsigned long durationUs = durationMinute * 1000000 * (60 - durationMinute);
    esp_sleep_enable_timer_wakeup(durationUs);
    esp_deep_sleep_start();
}

bool scheduler::sendBuffer(TMRInstrumentWeb *cloud, configReader *conf)
{
    uint16_t RTCMinute = conf->getMinute();
    Serial.print("RTC Minute Time:");
    Serial.println(RTCMinute);
    Serial.print("Time condition:");
    Serial.println(RTCMinute >= 50);

    if (RTCMinute >= 50 && RTCMinute <= 55) /// adjust the timer to trigger buffer sending cycle
    {
        int statusSendingBuffer = 0;
        Serial.println("saatnya kirim buffer");

        if (microSD->fileExists("/TMRBuffer.csv"))
        {
            microSD->copyFileByLines(microSD->card, "/TMRBuffer.csv", "/sendTMP.csv", 30);
            String bufferData = "";
            microSD->readFile(microSD->card, "/sendTMP.csv", &bufferData);

            if (bufferData.length() > 10)
            {
                Serial.println("mengirim buffer");
                String tz = "";
                if (conf->getTimeZone().toInt() < 10)
                {
                    tz += "+0" + conf->getTimeZone() + ":00";
                }
                else
                {
                    tz += "+" + conf->getTimeZone() + ":00";
                }
                statusSendingBuffer = cloud->processCSV(&bufferData, tz);
                if (statusSendingBuffer > 0)
                {
                    microSD->substractFile(microSD->card, "/TMRBuffer.csv", "/sendTMP.csv");
                    microSD->deleteFile(microSD->card, "/sendTMP.csv");
                    return true;
                }
                else
                {
                    microSD->deleteFile(microSD->card, "/sendTMP.csv");
                    return false;
                }
            }
            else
            {
                Serial.println("buffer kosong");
                return false;
            }
        }
        else
        {
            microSD->writeFile(microSD->card, "/TMRBuffer.csv", "");
            return false;
        }
    }
    else
    {
        return false;
    }
}