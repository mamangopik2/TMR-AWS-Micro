#define USE_SD_LOG 1
#include <Arduino.h>
#include <TMRSensor.h>
#include <WiFi.h>
#include <wifiManager.h>
#include <TMRemoteMQ.h>
#include <CSVLogger.h>
#include <esp_task_wdt.h>
#include <TMRLicenseManager.h>
#include <serialTool.h>

String sensorDataPacket;
uint8_t secondCounter = 0;
uint8_t runUpTimeMinute = 2;
unsigned long minuteCounter = 0;
unsigned long t0 = 0;
unsigned long logger_interval = 0;
uint8_t readyToLog = 0;
String fileLists;
String deviceInfo;

modbusSensor mbInstrument;       // define modbus Instrument
sensorManager instrumentManager; // define sensor manager
wifiManager networkManager;      // network Interface Manager
configReader sensorConfigurator;
TMRInstrumentWeb cloud;
scheduler systemScheduler;
TMRemoteMQ remote;
CSVLogger logger;
TMRLicenseManager licensing;
serialTool systemSerial;

String jsonString;

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

String getDeviceInfo()
{
  StaticJsonDocument<128> info;
  info["battery_voltage"] = 3.7;
  info["SN"] = "SN12345678";
  info["site"] = sensorConfigurator._siteName;
  info["plant"] = sensorConfigurator._plantName;
  info["device_name"] = sensorConfigurator._deviceName;
  info["device_time"] = sensorConfigurator.getISOTimeRTC();
  info["local_IP"] = WiFi.localIP();

  String jsonString;
  serializeJson(info, jsonString);
  return jsonString;
}

void setup()
{
  setCpuFrequencyMhz(240);
  esp_task_wdt_init(0xffffffff, true);
  Serial.setRxBufferSize(4096);
  Serial.begin(115200); // host serial
  systemSerial.hostSerial = &Serial;
  systemSerial.startThread(8192, 1, 1);

  xTaskCreatePinnedToCore(clock, "time scheduler", 1024, NULL, 6, &timeTask, 1);

  networkManager.microSD = logger.microSD;

  networkManager.logFilelist = &fileLists;
  networkManager.deviceInfo = &deviceInfo;
  networkManager.startThread(4096, 5, 0);

  sensorConfigurator.loadFile();               // load sensors conf
  sensorConfigurator.loadSerialConfigFile();   // load serial comm conf
  sensorConfigurator.loadSiteInfo();           // load site conf
  sensorConfigurator.loadTimeInfo();           // load time conf
  sensorConfigurator.loadCloudInfo();          // load cloud conf
  sensorConfigurator.conFigureSerial(&Serial); // run the configuration

  remote.deviceInfo = &deviceInfo;
  remote.logFileList = &fileLists;
  remote.setNetManager(&networkManager);
  remote.configurationManager = &sensorConfigurator;
  remote.handledSensorMessage = &sensorDataPacket;
  remote.startThread(24576, 4, 1);

  logger.init();
  logger.configurationManager = &sensorConfigurator;
  logger.handledLoggingMessage = &sensorDataPacket;
  logger.loggingFlag = &readyToLog;
  logger.startThread(4096, 1, 1);

  mbInstrument.init(&Serial2); // init the modbus instrument

  cloud.setHost(sensorConfigurator.getCloudHost().c_str());
  cloud.begin(sensorConfigurator.getCloudToken().c_str());
  bool initTime = true;
  sensorConfigurator.checkTimeUpdate(&initTime);
  instrumentManager.initAnalog(GAIN_TWOTHIRDS);
  cloud.reqWorkSpace();
  // networkManager.csvLogger = &logger; // pass the logger refference to the net manager to give csv files access
}

void loop() // this loop runs on Core1 by default
{
unlicensed:
  sensorDataPacket = sensorConfigurator.getSensorsValue(instrumentManager, mbInstrument); // sensor data packet which will be send to the cloud
  networkManager.globalMessage = sensorDataPacket;                                        // set the broadcast message for handling sensor value viewer
  sensorConfigurator.checkUpdate(&networkManager.sensorUpdated);                          // check for update if there any changes on the sensor data conf
  sensorConfigurator.checkSerialUpdate(&networkManager.serialComUpdated, mbInstrument);   // check for update if there any changes on the serial comm conf
  sensorConfigurator.checkSiteUpdate(&networkManager.siteUpdated);                        // check for update if there any changes on the site conf
  sensorConfigurator.checkTimeUpdate(&networkManager.timeUpdated);                        // check for update if there any changes on the time conf
  sensorConfigurator.checkCloudUpdate(&networkManager.cloudUpdated, &cloud);              // check for update if there any changes on the cloud conf if update occurs then update the cloud setup
  sensorConfigurator.checkRTCUpdate(&networkManager.RTCUpdated, &networkManager);         // check for update if there any changes on the RTC Configuration update

  // Serial.print("Original:");
  // Serial.println("00:1A:2B:3C:4D:5E");
  // Serial.print("encrypted:");
  // Serial.println("GH=bH@fJ^$N{En~.`");
  // Serial.print("decrypted:");
  // Serial.println(licensing.decrypt("GH=bH@fJ^$N{En~.`"));

  uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes
  Serial.print("Free Heap: ");
  Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
  Serial.println(" KB");

  if (remote.fail_counter > 20 || cloud.fail_counter > 10)
  {
    ESP.restart();
  }

  vTaskDelay(5000 / portTICK_PERIOD_MS); // delay

  if (!licensing.checkLicense())
  {
    goto unlicensed;
  }

  try
  {
    systemScheduler.manage(sensorDataPacket,
                           &sensorConfigurator,
                           &networkManager,
                           &cloud,
                           &runUpTimeMinute,
                           &minuteCounter, &readyToLog);
    fileLists = logger.microSD->listDir(logger.microSD->card, "/", 1);
    deviceInfo = getDeviceInfo();
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }
}