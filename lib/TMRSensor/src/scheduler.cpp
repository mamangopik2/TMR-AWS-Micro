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

void scheduler::checkRegisterSchedule(ModbusRTU *_modbusInstance, unsigned long currentUnixTimestamp, unsigned long memoryTimer, unsigned long *storeTimer)
{
    for (uint16_t i = 0; i < *registerCount; i++)
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

        if (registers[i][3] == RST_MINUTELY)
        {
            if (minute_elapsed(currentUnixTimestamp, memoryTimer))
            {
                resetRegisterScheduler(_modbusInstance, registers[i][0], registers[i][1], registers[i][2]);
                *storeTimer = currentUnixTimestamp; // update the store timer
            }
        }
        else if (registers[i][3] == RST_HOURLY)
        {
            if (hour_elapsed(currentUnixTimestamp, memoryTimer))
            {
                resetRegisterScheduler(_modbusInstance, registers[i][0], registers[i][1], registers[i][2]);
                *storeTimer = currentUnixTimestamp; // update the store timer
            }
        }
        else if (registers[i][3] == RST_DAILY)
        {
            if (day_elapsed(currentUnixTimestamp, memoryTimer))
            {
                resetRegisterScheduler(_modbusInstance, registers[i][0], registers[i][1], registers[i][2]);
                *storeTimer = currentUnixTimestamp; // update the store timer
            }
        }
        else if (registers[i][3] == RST_MONTHLY)
        {
            if (month_elapsed(currentUnixTimestamp, memoryTimer))
            {
                resetRegisterScheduler(_modbusInstance, registers[i][0], registers[i][1], registers[i][2]);
                *storeTimer = currentUnixTimestamp; // update the store timer
            }
        }
        else if (registers[i][3] == RST_YEARLY)
        {
            if (year_elapsed(currentUnixTimestamp, memoryTimer))
            {
                resetRegisterScheduler(_modbusInstance, registers[i][0], registers[i][1], registers[i][2]);
                *storeTimer = currentUnixTimestamp; // update the store timer
            }
        }
        else
        {
            Serial.println("[Scheduler] Unknown reset routine, skipping...");
        }
    }
}

bool ::scheduler::minute_elapsed(time_t currenTimeStamp, time_t past)
{
    return difftime(currenTimeStamp, past) >= 60;
}
bool ::scheduler::hour_elapsed(time_t currenTimeStamp, time_t past)
{
    return difftime(currenTimeStamp, past) >= 3600;
}
bool ::scheduler::day_elapsed(time_t currenTimeStamp, time_t past)
{
    return difftime(currenTimeStamp, past) >= 86400;
}
bool ::scheduler::week_elapsed(time_t currenTimeStamp, time_t past)
{
    return difftime(currenTimeStamp, past) >= 604800;
}
bool ::scheduler::month_elapsed(time_t currenTimeStamp, time_t past)
{
    struct tm *currentTime = localtime(&currenTimeStamp);
    struct tm *pastTime = localtime(&past);

    int yearDiff = currentTime->tm_year - pastTime->tm_year;
    int monthDiff = currentTime->tm_mon - pastTime->tm_mon + (yearDiff * 12);

    return monthDiff >= 1;
}
bool ::scheduler::year_elapsed(time_t currenTimeStamp, time_t past)
{
    struct tm *currentTime = localtime(&currenTimeStamp);
    struct tm *pastTime = localtime(&past);

    return (currentTime->tm_year - pastTime->tm_year) >= 1;
}

bool scheduler::checkMinutelyInterval(unsigned long currentUnixTimestamp, unsigned long storeTime)
{
    // Calculate elapsed time
    unsigned long deltaTime = currentUnixTimestamp - storeTime;

    // Serial.println("=============================================");
    // Serial.print("Current Unix Time: ");
    // Serial.println(currentUnixTimestamp);
    // Serial.print("Last Unix Time: ");
    // Serial.println(storeTime);
    // Serial.print("Delta Time: ");
    // Serial.println(deltaTime);
    // Serial.println("=============================================");

    // Check interval
    if (deltaTime >= 60)
    {
        Serial.println("[Scheduler] Interval met!");
        return true;
    }

    return false;
}

bool scheduler::checkHourlyInterval(unsigned long currentUnixTimestamp, unsigned long storeTime)
{
    // Calculate elapsed time
    unsigned long deltaTime = currentUnixTimestamp - storeTime;
    // Serial.println("=============================================");
    // Serial.print("Current Unix Time: ");
    // Serial.println(currentUnixTimestamp);
    // Serial.print("Last Unix Time: ");
    // Serial.println(storeTime);
    // Serial.print("Delta Time: ");
    // Serial.println(deltaTime);
    // Serial.println("=============================================");
    // Check interval
    if (deltaTime >= 3600)
    {
        Serial.println("[Scheduler] Hourly interval met!");
        return true;
    }

    return false;
}

bool scheduler::checkDailyInterval(unsigned long currentUnixTimestamp, unsigned long storeTime)
{
    // Calculate elapsed time
    unsigned long deltaTime = currentUnixTimestamp - storeTime;
    // Serial.println("=============================================");
    // Serial.print("Current Unix Time: ");
    // Serial.println(currentUnixTimestamp);
    // Serial.print("Last Unix Time: ");
    // Serial.println(storeTime);
    // Serial.print("Delta Time: ");
    // Serial.println(deltaTime);
    // Serial.println("=============================================");
    // Check interval
    if (deltaTime >= 86400)
    {
        Serial.println("[Scheduler] Daily interval met!");
        return true;
    }

    return false;
}

bool scheduler::checkMonthlyInterval(unsigned long currentUnixTimestamp, unsigned long storeTime)
{
    // Calculate elapsed time
    unsigned long deltaTime = currentUnixTimestamp - storeTime;
    // Serial.println("=============================================");
    // Serial.print("Current Unix Time: ");
    // Serial.println(currentUnixTimestamp);
    // Serial.print("Last Unix Time: ");
    // Serial.println(storeTime);
    // Serial.print("Delta Time: ");
    // Serial.println(deltaTime);
    // Serial.println("=============================================");
    // Check interval
    if (deltaTime >= 2592000) // 30 days
    {
        Serial.println("[Scheduler] Monthly interval met!");
        return true;
    }

    return false;
}

bool scheduler::checkYearlyInterval(unsigned long currentUnixTimestamp, unsigned long storeTime)
{
    // Calculate elapsed time
    unsigned long deltaTime = currentUnixTimestamp - storeTime;
    // Serial.println("=============================================");
    // Serial.print("Current Unix Time: ");
    // Serial.println(currentUnixTimestamp);
    // Serial.print("Last Unix Time: ");
    // Serial.println(storeTime);
    // Serial.print("Delta Time: ");
    // Serial.println(deltaTime);
    // Serial.println("=============================================");
    // Check interval
    if (deltaTime >= 31536000) // 365 days
    {
        Serial.println("[Scheduler] Yearly interval met!");
        return true;
    }

    return false;
}

void scheduler::deepSleep(unsigned long durationMinute)
{
    digitalWrite(PHERI_SLEEP_PIN, LOW);
    unsigned long durationUs = durationMinute * 1000000 * (60 - durationMinute);
    esp_sleep_enable_timer_wakeup(durationUs);
    esp_deep_sleep_start();
}
