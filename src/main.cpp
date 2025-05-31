#include <Arduino.h>
#include <TMRSensor.h>
#include <WiFi.h>
#include <wifiManager.h>
#include <TMRemoteMQ.h>

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
TMRemoteMQ remote;

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

TaskHandle_t remoteTasks;

void messageReceived(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == remote._SN + "/TMRAWS/device/beacon")
  {
    networkManager.beaconUpdatedTime = millis(); // update the beacon time to prevent controller entering sleep mode
  }
  if (topic == remote._SN + "/TMRAWS/device/sensor/update")
  {
    File file;
    file = SPIFFS.open("/sensor.json", "w", true);
    file.print(payload);
    file.close();
    networkManager.sensorUpdated = true; // set update flag to trigger the update
  }
  if (topic == remote._SN + "/TMRAWS/device/cloud/set")
  {
    File file;
    file = SPIFFS.open("/cloud_config.json", "w", true);
    file.print(payload);
    file.close();
    networkManager.cloudUpdated = true; // set update flag to trigger the update
  }
  if (topic == remote._SN + "/TMRAWS/device/time/set")
  {
    File file;
    file = SPIFFS.open("/time_config.json", "w", true);
    file.print(payload);
    file.close();
    networkManager.timeUpdated = true; // set update flag to trigger the update
  }
  if (topic == remote._SN + "/TMRAWS/device/site/set")
  {
    File file;
    file = SPIFFS.open("/site_config.json", "w", true);
    file.print(payload);
    file.close();
    networkManager.siteUpdated = true; // set update flag to trigger the update
  }
  if (topic == remote._SN + "/TMRAWS/device/sensor/add")
  {
    File myJsonFile;
    myJsonFile = SPIFFS.open("/sensor.json", "r");
    String jsonStringFromFile = myJsonFile.readString();
    myJsonFile.close();

    String jsonStringFromClient = payload;
    String newJsonData = networkManager.fuseSensor(jsonStringFromFile, jsonStringFromClient);

    Serial.print("from file");
    Serial.println(jsonStringFromFile);
    Serial.print("from client");
    Serial.println(jsonStringFromClient);
    Serial.print("fused");
    Serial.println(newJsonData);

    myJsonFile = SPIFFS.open("/sensor.json", "w", true);
    myJsonFile.print(newJsonData);
    myJsonFile.close();
    networkManager.sensorUpdated = true; // set update flag to trigger the update
  }
}

void remoteLoop(void *param)
{
  remote.begin("broker.hivemq.com", 1883, "TMR-123");
  unsigned long t1 = millis();
  while (1)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (!remote.mqtt_client->connected())
      {
        remote.connect();
        remote.mqtt_client->onMessage(messageReceived);
      }
    }
    remote.mqtt_client->loop();
    if (millis() - t1 >= 2000)
    {
      String topic = remote._SN + "/TMRAWS/device/sensor/data";
      bool status = remote.mqtt_client->publish(topic.c_str(), sensorDataPacket);
      Serial.print("publish to:");
      Serial.println(topic);

      topic = remote._SN + "/TMRAWS/device/status";
      status = remote.mqtt_client->publish(topic.c_str(), "ping");
      Serial.print("publish to:");
      Serial.println(topic);

      topic = remote._SN + "/TMRAWS/device/sensor/list";
      status = remote.mqtt_client->publish(topic.c_str(), sensorConfigurator._jsonString);
      Serial.print("publish to:");
      Serial.println(topic);
      t1 = millis();
    }
    vTaskDelay(100);
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
  xTaskCreatePinnedToCore(remoteLoop, "remote", 8192, NULL, 4, &remoteTasks, 1);

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
  instrumentManager.initAnalog(GAIN_TWOTHIRDS);
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