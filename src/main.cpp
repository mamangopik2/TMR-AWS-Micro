#include <Arduino.h>
#include <TMRSensor.h>
#include <WiFi.h>
#include <wifiManager.h>

String sensorDataPacket;
uint8_t secondCounter = 0;
uint8_t runUpTimeMinute = 2;
unsigned long minuteCounter = 0;
unsigned long t0 = 0;

modbusSensor mbInstrument;       // define modbus Instrument
sensorManager instrumentManager; // define sensor manager
wifiManager networkManager;      // network Interface Manager
configReader sensorConfigurator;
TMRInstrumentWeb cloud;

String jsonString;

TaskHandle_t NetManagerTasks;
void netManagerRoutine(void *param)
{
  networkManager.begin();
  networkManager.wifiTimeout = 5000;
  networkManager.enableDHCP = true;          // default value is false
  networkManager.automaticAPDisable = false; // default is true, after connected to WiFi the AP is disabled
  while (1)
  {
    networkManager.run();
    vTaskDelay(1);
  }
}

TaskHandle_t timeTask;
void timeScheduler(void *param)
{

  t0 = millis();
  while (1)
  {
    if (millis() - t0 > 850)
    {
      secondCounter++;
      t0 = millis();
    }

    if (secondCounter >= 60)
    {
      secondCounter = 0;
      minuteCounter++;
    }

    vTaskDelay(500);
  }
}

void deepSleep(unsigned long durationMinute)
{
  unsigned long durationUs = durationMinute * 1000000 * (60 - durationMinute);
  esp_sleep_enable_timer_wakeup(durationUs);
  Serial.flush();
  esp_deep_sleep_start();
}

void setup()
{
  // Serial.begin(115200); // host serial
  xTaskCreatePinnedToCore(netManagerRoutine, "network manager", 8192, NULL, 5, &NetManagerTasks, 0);
  xTaskCreatePinnedToCore(timeScheduler, "time scheduler", 1024, NULL, 6, &timeTask, 0);

  sensorConfigurator.loadFile();               // load sensors conf
  sensorConfigurator.loadSerialConfigFile();   // load serial comm conf
  sensorConfigurator.loadSiteInfo();           // load site conf
  sensorConfigurator.loadTimeInfo();           // load time conf
  sensorConfigurator.loadCloudInfo();          // load cloud conf
  sensorConfigurator.conFigureSerial(&Serial); // run the configuration

  mbInstrument.init(&Serial); // init the modbus instrument

  cloud.begin(sensorConfigurator.getCloudToken().c_str());
  cloud.setHost(sensorConfigurator.getCloudHost().c_str());

  // debuger.setHost("192.168.15.83");
  // debuger.setPort(1234);
}

void loop() // this loop runs on Core1 by default
{
  sensorDataPacket = sensorConfigurator.getSensorsValue(instrumentManager, mbInstrument); // sensor data packet which will be send to the cloud
  networkManager.globalMessage = sensorDataPacket;                                        // set the broadcast message for handling sensor value viewer
  sensorConfigurator.checkUpdate(&networkManager.sensorUpdated);                          // check for update if there any changes on the sensor data conf
  sensorConfigurator.checkSerialUpdate(&networkManager.serialComUpdated, mbInstrument);   // check for update if there any changes on the serial comm conf
  sensorConfigurator.checkSiteUpdate(&networkManager.siteUpdated);                        // check for update if there any changes on the site conf
  sensorConfigurator.checkTimeUpdate(&networkManager.timeUpdated);                        // check for update if there any changes on the time conf
  sensorConfigurator.checkCloudUpdate(&networkManager.cloudUpdated, &cloud);              // check for update if there any changes on the cloud conf if update occurs then update the cloud setup
  vTaskDelay(300 / portTICK_PERIOD_MS);                                                   // delay

  Serial.print(minuteCounter);
  Serial.print(':');
  Serial.println(secondCounter);

  Serial.print("beacon Time:");
  Serial.println(networkManager.getBeaconTime());

  if (sensorConfigurator.getCloudInterval().toInt() >= 5) // if the sending interval > 5 minutes, activate sleep mode
  {
    if (minuteCounter >= runUpTimeMinute)
    {
      sensorConfigurator.postSensors(sensorDataPacket.c_str(), &cloud);
      if (millis() - networkManager.getBeaconTime() > (10 * 60 * 1000)) // activate sleep after 10 minutes from client finished configuration
      {
        deepSleep(sensorConfigurator.getCloudInterval().toInt() - runUpTimeMinute);
      }
      else
      {
        minuteCounter = 0;
      }
    }
  }
  else
  {
    // send every x minutes without sleep();
    if (minuteCounter >= sensorConfigurator.getCloudInterval().toInt())
    {
      sensorConfigurator.postSensors(sensorDataPacket.c_str(), &cloud);
      minuteCounter = 0; // reset the minute timer after send the data
    }
  }
}