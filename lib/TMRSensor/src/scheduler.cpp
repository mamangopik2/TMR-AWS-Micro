#include "TMRSensor.h"
void scheduler::manage(const String &data, configReader *conf, wifiManager *networkManager, TMRInstrumentWeb *cloud, uint8_t *runUpTimeMinute, unsigned long *clockMinute, uint8_t *logFlag)
{
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
                    status = cloud->publishBulk(data, conf->getISOTimeNTP());
                }
                else
                {
                    *logFlag = 1;
                    status = cloud->publishBulk(data, conf->getISOTimeRTC());
                }
                if (status == true)
                {
                    // Serial.println("deep sleep");
                    *logFlag = 1;
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
                cloud->publishBulk(data, conf->getISOTimeNTP());
            }
            else
            {
                *logFlag = 1;
                cloud->publishBulk(data, conf->getISOTimeRTC());
            }
            *clockMinute = 0; // reset the minute timer after send the data
        }
    }
}

void scheduler::deepSleep(unsigned long durationMinute)
{
    unsigned long durationUs = durationMinute * 1000000 * (60 - durationMinute);
    esp_sleep_enable_timer_wakeup(durationUs);
    // Serial.flush();
    esp_deep_sleep_start();
}
