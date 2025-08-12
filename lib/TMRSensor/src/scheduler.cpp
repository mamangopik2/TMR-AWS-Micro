#include "TMRSensor.h"
void scheduler::manage(String *data, configReader *conf, wifiManager *networkManager, TMRInstrumentWeb *cloud, uint8_t *runUpTimeMinute, unsigned long *clockMinute, uint8_t *logFlag)
{
    uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes
    bool status;
    if (conf->getCloudInterval().toInt() >= 5) // if the sending interval > 5 minutes, activate sleep mode
    {
        if (*clockMinute >= *runUpTimeMinute)
        {
            // Serial.print("sending, Timesource :");
            // Serial.println(conf->getTimeSource());
            if (millis() - networkManager->getBeaconTime() > (1 * 60 * 1000)) // activate sleep after 10 minutes from client finished configuration
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
                        ESP.restart();
                    }
                    status = cloud->publishBulk(*data, conf->getISOTimeNTP());
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
                        ESP.restart();
                    }
                    status = cloud->publishBulk(*data, conf->getISOTimeRTC());
                }
                if (status == true)
                {
                    // Serial.println("deep sleep");
                    deepSleep(conf->getCloudInterval().toInt() - *runUpTimeMinute);
                }
                else
                {
                    *clockMinute = 0;
                }
            }
            else
            {
                *clockMinute = 0;
            }
        }
    }
    else
    {
        // Serial.print("sending, Timesource :");
        // Serial.println(conf->getTimeSource());
        //  send every x minutes without sleep();
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
                    ESP.restart();
                }
                cloud->publishBulk(*data, conf->getISOTimeNTP());
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
                    ESP.restart();
                }
                cloud->publishBulk(*data, conf->getISOTimeRTC());
            }
            *clockMinute = 0; // reset the minute timer after send the data
        }
    }
}

void scheduler::resetRegisterScheduler(unsigned long *currentUnixTimestamp, ModbusRTU *_modbusInstance, uint64_t slaveAddress, uint16_t regOffset, uint16_t regAddr)
{
    // reset the register scheduler
    uint16_t val = 0;
    _modbusInstance->writeHreg(slaveAddress, regOffset + regAddr, &val);
}

bool scheduler::checkMinutelyInterval(unsigned long *sourceTimer, unsigned long *storeTimer)
{
    if (*sourceTimer - *storeTimer >= 60)
    {
        *storeTimer = *sourceTimer;
        return true;
    }
    return false;
}
bool scheduler::checkHourlyInterval(unsigned long *sourceTimer, unsigned long *storeTimer)
{
    if (*sourceTimer - *storeTimer >= 3600)
    {
        *storeTimer = *sourceTimer;
        return true;
    }
    return false;
}
bool scheduler::checkDaylyInterval(unsigned long *sourceTimer, unsigned long *storeTimer)
{
    if (*sourceTimer - *storeTimer >= 86400)
    {
        *storeTimer = *sourceTimer;
        return true;
    }
    return false;
}
bool scheduler::checkMonthlyInterval(unsigned long *sourceTimer, unsigned long *storeTimer)
{
    if (*sourceTimer - *storeTimer >= 2592000) // 30 days
    {
        *storeTimer = *sourceTimer;
        return true;
    }
    return false;
}
bool scheduler::checkYearlyInterval(unsigned long *sourceTimer, unsigned long *storeTimer)
{
    if (*sourceTimer - *storeTimer >= 31536000) // 365 days
    {
        *storeTimer = *sourceTimer;
        return true;
    }
    return false;
}

void scheduler::deepSleep(unsigned long durationMinute)
{
    unsigned long durationUs = durationMinute * 1000000 * (60 - durationMinute);
    esp_sleep_enable_timer_wakeup(durationUs);
    // Serial.flush();
    esp_deep_sleep_start();
}
