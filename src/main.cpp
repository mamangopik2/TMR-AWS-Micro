#define USE_SD_LOG 1

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Arduino.h>
#include <TMRSensor.h>
#include <WiFi.h>
#include <wifiManager.h>
#include <TMRemoteMQ.h>
#include <CSVLogger.h>
#include <esp_task_wdt.h>
#include <TMRLicenseManager.h>
#include <serialTool.h>
#include <EEPROM.h>

RTC_DATA_ATTR unsigned long currentULPUnixTimestamp = 0; // store data on RTC memory
RTC_DATA_ATTR unsigned long ULPminuteTimerCounter = 0;   // store data on RTC memory
RTC_DATA_ATTR unsigned long ULPHourTimerCounter = 0;     // store data on RTC memory
RTC_DATA_ATTR unsigned long ULPDayTimerCounter = 0;      // store data on RTC memory
RTC_DATA_ATTR unsigned long ULPMonthTimerCounter = 0;    // store data on RTC memory
RTC_DATA_ATTR unsigned long ULPYearTimerCounter = 0;     // store data on RTC memory

String sensorDataPacket;
uint8_t secondCounter = 0;
uint8_t runUpTimeMinute = 2;
unsigned long minuteCounter = 0;
unsigned long t0 = 0;
unsigned long logger_interval = 0;
uint8_t readyToLog = 0;
String fileLists;
String deviceInfo;
String jsonString;
String AIReadData;

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
  info["battery_voltage"] = String(((analogRead(VBAT_SENSE_PIN) * (3.3 / 4095.0)) * 2) + 0.29, 2); // read the battery voltage
  info["sn"] = "SN12345678";
  info["site"] = sensorConfigurator._siteName;
  info["plant"] = sensorConfigurator._plantName;
  info["device_name"] = sensorConfigurator._deviceName;
  info["device_time"] = sensorConfigurator.getISOTimeRTC();
  info["free_heap"] = ESP.getFreeHeap() / 1024.0;
  info["local_IP"] = WiFi.localIP();
  info["mac_address"] = WiFi.macAddress();
  info["device_id"] = WiFi.localIP();

  String jsonString;
  serializeJson(info, jsonString);
  info.clear();
  return jsonString;
}

void setup()
{
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  setCpuFrequencyMhz(80);
  esp_task_wdt_init(0xffffffff, true);
  pinMode(PHERI_SLEEP_PIN, OUTPUT);
  pinMode(VBAT_SENSE_PIN, INPUT);      // set the VBAT sense pin as input
  digitalWrite(PHERI_SLEEP_PIN, HIGH); // set the sleep pin to high because it is driving NPN transistor
  Serial.setRxBufferSize(1024);
  Serial.begin(115200); // host serial
  EEPROM.begin(1024);   // initialize EEPROM with 1024 bytes
  licensing.begin();    // initialize the license manager
  systemSerial.hostSerial = &Serial;
  systemSerial.AIReadData = &AIReadData;                     // set the AI read data to the serial tool
  systemSerial.aiRawData = instrumentManager.analogReadData; // set the sensor manager to the serial tool
  systemSerial.deviceInformation = &deviceInfo;              // set the device information to the serial tool
  systemSerial.license = &licensing;                         // set the license manager to the serial tool
  systemSerial.startThread(4096, 1, 1);

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
  remote.startThread((16 * 1024), 4, 1);

  logger.init();
  logger.configurationManager = &sensorConfigurator;
  logger.handledLoggingMessage = &sensorDataPacket;
  logger.loggingFlag = &readyToLog;
  logger.startThread(4096, 1, 1);

  mbInstrument.init(&Serial2); // init the modbus instrument

  systemScheduler.registerCount = &sensorConfigurator.modbusRegistersCount; // set the register count to the scheduler
  systemScheduler.registers = sensorConfigurator.modbusHREGS;               // set the

  cloud.setHost(sensorConfigurator.getCloudHost().c_str());
  cloud.begin(sensorConfigurator.getCloudToken().c_str());
  bool initTime = true;
  try
  {
    sensorConfigurator.checkTimeUpdate(&initTime);
    instrumentManager.initAnalog(0x48, GAIN_TWOTHIRDS);
  }
  catch (const std::exception &e)
  {
  }

  cloud.reqWorkSpace();

  cloud.sensorConfiguration = &sensorConfigurator._jsonString;

  Wire.begin();
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("Done.\n");
}

void loop() // this loop runs on Core1 by default
{
testTimer:
  unsigned long lookupTimer;
  currentULPUnixTimestamp = sensorConfigurator.getUnixTime();                                                              // set the RTC unix time to the current unix time
  lookupTimer = ULPminuteTimerCounter;                                                                                     // get the current unix time
  systemScheduler.checkRegisterSchedule(mbInstrument._modbusInstance, currentULPUnixTimestamp, lookupTimer, &lookupTimer); // check the register schedule
  ULPminuteTimerCounter = lookupTimer;                                                                                     // update the minute timer counter

  lookupTimer = ULPHourTimerCounter;                                                                                       // get the current hour timer
  systemScheduler.checkRegisterSchedule(mbInstrument._modbusInstance, currentULPUnixTimestamp, lookupTimer, &lookupTimer); // check the register schedule
  ULPHourTimerCounter = lookupTimer;                                                                                       // update the hour timer counter

  lookupTimer = ULPDayTimerCounter;                                                                                        // get the current day timer
  systemScheduler.checkRegisterSchedule(mbInstrument._modbusInstance, currentULPUnixTimestamp, lookupTimer, &lookupTimer); // check the register schedule
  ULPDayTimerCounter = lookupTimer;                                                                                        // update the day timer counter

  lookupTimer = ULPMonthTimerCounter;                                                                                      // get the current month timer
  systemScheduler.checkRegisterSchedule(mbInstrument._modbusInstance, currentULPUnixTimestamp, lookupTimer, &lookupTimer); // check the register schedule
  ULPMonthTimerCounter = lookupTimer;                                                                                      // update the month timer counter

  lookupTimer = ULPYearTimerCounter;                                                                                       // get the current year timer
  systemScheduler.checkRegisterSchedule(mbInstrument._modbusInstance, currentULPUnixTimestamp, lookupTimer, &lookupTimer); // check the register schedule
  ULPYearTimerCounter = lookupTimer;                                                                                       // update the year timer counter

  Serial.print("Current Unix Time: ");
  Serial.println(currentULPUnixTimestamp);
  Serial.print("Minute Timer: ");
  Serial.println(ULPminuteTimerCounter);
  Serial.print("DeltaMinuteTimer: ");
  Serial.println(currentULPUnixTimestamp - ULPminuteTimerCounter);

  Serial.print("Hour Timer: ");
  Serial.println(ULPHourTimerCounter);
  Serial.print("DeltaHourTimer: ");
  Serial.println(currentULPUnixTimestamp - ULPHourTimerCounter);
  Serial.print("Day Timer: ");
  Serial.println(ULPDayTimerCounter);
  Serial.print("DeltaDayTimer: ");

unlicensed:
  sensorDataPacket = sensorConfigurator.getSensorsValue(instrumentManager, mbInstrument); // sensor data packet which will be send to the cloud
  networkManager.globalMessage = &sensorDataPacket;                                       // set the broadcast message for handling sensor value viewer
  sensorConfigurator.checkUpdate(&networkManager.sensorUpdated);                          // check for update if there any changes on the sensor data conf
  sensorConfigurator.checkSerialUpdate(&networkManager.serialComUpdated, mbInstrument);   // check for update if there any changes on the serial comm conf
  sensorConfigurator.checkSiteUpdate(&networkManager.siteUpdated);                        // check for update if there any changes on the site conf
  sensorConfigurator.checkTimeUpdate(&networkManager.timeUpdated);                        // check for update if there any changes on the time conf
  sensorConfigurator.checkCloudUpdate(&networkManager.cloudUpdated, &cloud);              // check for update if there any changes on the cloud conf if update occurs then update the cloud setup
  sensorConfigurator.checkRTCUpdate(&networkManager.RTCUpdated, &networkManager);         // check for update if there any changes on the RTC Configuration update

  uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes
  Serial.print("Free Heap: ");
  Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
  Serial.println(" KB");

  vTaskDelay(3000 / portTICK_PERIOD_MS); // delay

  if (!licensing.checkLicense())
  {
    goto unlicensed;
  }

  try
  {
    systemScheduler.manage(networkManager.globalMessage,
                           &sensorConfigurator,
                           &networkManager,
                           &cloud,
                           &runUpTimeMinute,
                           &minuteCounter, &readyToLog);

    fileLists = logger.microSD->listDir(SD_MMC, "/", 1);
    deviceInfo = getDeviceInfo();
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }
}