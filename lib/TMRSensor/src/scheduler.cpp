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

void scheduler::resetRegisterScheduler(ModbusRTU *_modbusInstance, uint64_t slaveAddress, uint16_t regOffset, uint16_t regAddr)
{
    // reset the register scheduler
    uint16_t val = 0;
    _modbusInstance->writeHreg(slaveAddress, regOffset + regAddr, &val);
}

void scheduler::resetRegisterByFlag(ModbusRTU *_modbusInstance, byte *flag, uint16_t nufOfreg)
{
    Serial.print("registerCount:");
    Serial.println(nufOfreg);
    for (uint16_t i = 0; i < nufOfreg; i++)
    {
        // Serial.print("Register Lists: ");
        // Serial.print("Slave ID: ");
        // Serial.print(registers[i][0]);
        // Serial.print(" Reg Offset: ");
        // Serial.print(registers[i][1]);
        // Serial.print(" Reg Addr: ");
        // Serial.print(registers[i][2]);
        // Serial.print(" Reset Routine: ");

        // if (registers[i][3] == RST_NONE)
        // {
        //     Serial.println("No Reset Routine");
        //     continue; // skip if no reset routine
        // }
        // else if (registers[i][3] == RST_MINUTELY)
        // {
        //     Serial.println("Minutely Reset Routine");
        // }
        // else if (registers[i][3] == RST_HOURLY)
        // {
        //     Serial.println("Hourly Reset Routine");
        // }
        // else if (registers[i][3] == RST_DAILY)
        // {
        //     Serial.println("Daily Reset Routine");
        // }
        // else if (registers[i][3] == RST_MONTHLY)
        // {
        //     Serial.println("Monthly Reset Routine");
        // }
        // else if (registers[i][3] == RST_YEARLY)
        // {
        //     Serial.println("Yearly Reset Routine");
        // }

        if (*flag == 1)
        {
            resetRegisterScheduler(_modbusInstance, registers[i][0], registers[i][1], registers[i][2]);
        }
    }
    *flag = 0;
}

void scheduler::timeComparison(uint16_t *tCur, uint16_t *tLast, byte *flag)
{
    if (*tLast == 0)
    {
        *tLast = *tCur;
    }
    Serial.print("Current:");
    Serial.print(*tCur);
    Serial.print(" Last:");
    Serial.println(*tLast);
    if (*tCur < *tLast)
    {
        *flag = 1;
    }
    *tLast = *tCur;
}

void scheduler::deepSleep(unsigned long durationMinute)
{
    digitalWrite(PHERI_SLEEP_PIN, LOW);
    unsigned long durationUs = durationMinute * 1000000 * (60 - durationMinute);
    esp_sleep_enable_timer_wakeup(durationUs);
    esp_deep_sleep_start();
}

bool scheduler::sendBuffer(TMRInstrumentWeb *cloud, configReader *conf, String *dataToSend)
{
    uint16_t RTCMinute = conf->getMinute();
    Serial.print("RTC Minute Time:");
    Serial.println(RTCMinute);
    Serial.print("Time condition:");
    Serial.println(RTCMinute >= 50);

    if (RTCMinute >= 50 && RTCMinute <= 55) /// adjust the timer to trigger buffer sending cycle
    {
        Serial.println("saatnya kirim buffer");
        int statusSendingBuffer = 0;
        if (dataToSend->length() > 10)
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

            /// cut the full string =============

            statusSendingBuffer = cloud->processCSV(dataToSend, tz);
            // ==============================================
            if (statusSendingBuffer > 0)
            {

                return true;
            }
            else
            {
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
        return false;
    }
}