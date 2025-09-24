#include "TMRSensor.h"
void scheduler::manage(String *data, configReader *conf, wifiManager *networkManager, TMRInstrumentWeb *cloud, uint8_t *runUpTimeMinute, unsigned long *clockMinute, uint8_t *logFlag)
{
    uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes
    bool status;
    if (conf->getCloudInterval().toInt() >= 5) // if the sending interval > 5 minutes, activate sleep mode
    {
        Serial.println("> 5 minutes");
        if (conf->getMinute() % conf->getCloudInterval().toInt() == 0)
        {
            Serial.print("sending, Timesource :");
            Serial.println(conf->getTimeSource());
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
                    Serial.print("sending, Timesource :");
                    Serial.println(conf->getTimeSource());
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
                    Serial.println("deep sleep");
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

void scheduler::resetRegisterScheduler(ModbusRTU *_modbusInstance, uint64_t slaveAddress, uint16_t regOffset, uint16_t regAddr)
{
    // reset the register scheduler
    uint16_t val = 0;
    _modbusInstance->writeHreg(slaveAddress, regOffset + regAddr, &val);
}

int getYearFromUnix(unsigned long unixTime)
{
    unsigned long secondsInYear = 31556926UL; // average seconds per year (365.2422 days)
    return 1970 + (unixTime / secondsInYear);
}

int getMonthFromUnix(unsigned long unixTime)
{
    unsigned long secondsInMonth = 2629743UL;    // average seconds per month (30.436875 days)
    return (unixTime / secondsInMonth) % 12 + 1; // +1 to convert from 0-11 to 1-12
}

int getDayFromUnix(unsigned long unixTime)
{
    unsigned long secondsInDay = 86400UL;      // seconds in a day
    return (unixTime / secondsInDay) % 31 + 1; // +1 to convert from 0-30 to 1-31
}

int getHourFromUnix(unsigned long unixTime)
{
    unsigned long secondsInHour = 3600UL;   // seconds in an hour
    return (unixTime / secondsInHour) % 24; // returns 0-23
}

int getMinuteFromUnix(unsigned long unixTime)
{
    unsigned long secondsInMinute = 60UL;     // seconds in a minute
    return (unixTime / secondsInMinute) % 60; // returns 0-59
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
