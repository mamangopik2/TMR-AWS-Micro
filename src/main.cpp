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
scheduler systemScheduler;

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
void clock(void *param)
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
  Serial.begin(115200); // host serial
  xTaskCreatePinnedToCore(netManagerRoutine, "network manager", 8192, NULL, 5, &NetManagerTasks, 0);
  xTaskCreatePinnedToCore(clock, "time scheduler", 1024, NULL, 6, &timeTask, 1);

  sensorConfigurator.loadFile();               // load sensors conf
  sensorConfigurator.loadSerialConfigFile();   // load serial comm conf
  sensorConfigurator.loadSiteInfo();           // load site conf
  sensorConfigurator.loadTimeInfo();           // load time conf
  sensorConfigurator.loadCloudInfo();          // load cloud conf
  sensorConfigurator.conFigureSerial(&Serial); // run the configuration

  mbInstrument.init(&Serial2); // init the modbus instrument

  cloud.setHost(sensorConfigurator.getCloudHost().c_str());
  cloud.begin(sensorConfigurator.getCloudToken().c_str());
  bool initTime = true;
  sensorConfigurator.checkTimeUpdate(&initTime);
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
  sensorConfigurator.checkRTCUpdate(&networkManager.RTCUpdated, &networkManager);
  vTaskDelay(500 / portTICK_PERIOD_MS); // delay

  systemScheduler.manage(sensorDataPacket,
                         &sensorConfigurator,
                         &networkManager,
                         &cloud,
                         &runUpTimeMinute,
                         &minuteCounter);

  Serial.print(minuteCounter);
  Serial.print(':');
  Serial.println(secondCounter);
  Serial.print("beacon Time:");
  Serial.println(networkManager.getBeaconTime());
  Serial.print("beacon Time:");
  Serial.print(millis() - networkManager.getBeaconTime());
  Serial.print("---thresshold Time:");
  Serial.println((1 * 60 * 1000));

  uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes
  Serial.print("Free Heap: ");
  Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
  Serial.println(" KB");
}