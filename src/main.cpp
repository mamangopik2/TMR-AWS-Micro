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

RTC_DATA_ATTR unsigned long ULPminuteTimerCounter = 0; // store data on RTC memory (unused for now)
RTC_DATA_ATTR unsigned long ULPHourTimerCounter = 0;   // store data on RTC memory (unused for now)
RTC_DATA_ATTR unsigned long ULPDayTimerCounter = 0;    // store data on RTC memory (unused for now)
RTC_DATA_ATTR unsigned long ULPMonthTimerCounter = 0;  // store data on RTC memory (unused for now)
RTC_DATA_ATTR unsigned long ULPYearTimerCounter = 0;   // store data on RTC memory (unused for now)

uint16_t tcnt0 = 0; // second counter
uint16_t tcnt1 = 0; // minute counter
uint16_t tcnt2 = 0; // hour counter
uint16_t tcnt3 = 0; // day counter

uint32_t lastMinutelyReset = 0;
uint32_t lastHourlyReset = 0;
uint32_t lastDailyReset = 0;

byte minRstFlag = 0;         // minutely reset flag
byte hourRstFlag = 0;        // hourly reset flag
byte dayRstFlag = 0;         // daily reset flag
byte monthRstFlag = 0;       // monthly reset flag
byte RTCSynced = 0;          // RTC synced flag
uint8_t readyToLog = 0;      // flag for local logging
uint8_t writeBufferFlag = 0; // flag for buffer logging

uint8_t secondCounter = 0;       // clock service
unsigned long minuteCounter = 0; // clock service
unsigned long t0 = 0;            // clock service

uint8_t runUpTimeMinute = 2; // time to run up before going to sleep
String fileLists;            // file list payload
String deviceInfo;           // device info payload
String jsonString;           // used for device info buffer
String AIReadData;           // used for utility tool payload
String sensorDataPacket;     // sensor data payload

modbusSensor ObjMBInstrument;       // define modbus Instrument
sensorManager ObjInstrumentManager; // define sensor manager
wifiManager ObjWebUI;               // network Interface Manager
configReader ObjSensorConfigurator; // configuration manager
TMRInstrumentWeb ObjCloud;          // cloud service manager
scheduler ObjScheduler;             // time and task scheduler
TMRemoteMQ ObjRemote;               // remote access
CSVLogger ObjLogger;                // data logging service
TMRLicenseManager ObjLicensing;     // license manager
serialTool ObjSystemSerial;         // system serial communication to utility tool

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
  info["sn"] = ObjRemote._SN;
  char bufferSerialOut[16];
  sprintf(bufferSerialOut, "%d:%d%d:%d%d:%d%d", tcnt3, tcnt2 / 10, tcnt2 % 10, tcnt1 / 10, tcnt1 % 10, tcnt0 / 10, tcnt0 % 10);
  info["up_time"] = bufferSerialOut;
  info["site"] = ObjSensorConfigurator._siteName;
  info["plant"] = ObjSensorConfigurator._plantName;
  info["device_name"] = ObjSensorConfigurator._deviceName;
  info["device_time"] = ObjSensorConfigurator.getISOTimeRTC();
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
  esp_task_wdt_init(0xffff, true);
  pinMode(PHERI_SLEEP_PIN, OUTPUT);
  pinMode(VBAT_SENSE_PIN, INPUT);      // set the VBAT sense pin as input
  digitalWrite(PHERI_SLEEP_PIN, HIGH); // set the sleep pin to high because it is driving NPN transistor
  Serial.setRxBufferSize(1024);
  Serial.begin(115200); // host serial
  EEPROM.begin(1024);   // initialize EEPROM with 1024 bytes
  ObjLicensing.begin(); // initialize the license manager
  ObjSystemSerial.hostSerial = &Serial;
  ObjSystemSerial.AIReadData = &AIReadData;                        // set the AI read data to the serial tool
  ObjSystemSerial.aiRawData = ObjInstrumentManager.analogReadData; // set the sensor manager to the serial tool
  ObjSystemSerial.deviceInformation = &deviceInfo;                 // set the device information to the serial tool
  ObjSystemSerial.license = &ObjLicensing;                         // set the license manager to the serial tool
  // systemSerial.startThread(4096, 1, 1);

  xTaskCreatePinnedToCore(clock, "time scheduler", 1024, NULL, 6, &timeTask, 1); // run clock service on core 1

  ObjWebUI.microSD = ObjLogger.microSD; // set microSD reference to the webUI
  ObjWebUI.logFilelist = &fileLists;    // set the log file list reference to the webUI
  ObjWebUI.deviceInfo = &deviceInfo;    // set the device info reference to the webUI
  ObjWebUI.startThread(4096, 5, 0);     // run webUI on core 0 (paralel)

  ObjSensorConfigurator.loadFile();               // load sensors conf file from SPIFFS
  ObjSensorConfigurator.loadSerialConfigFile();   // load serial comm conf file from SPIFFS
  ObjSensorConfigurator.loadSiteInfo();           // load site conf file from SPIFFS
  ObjSensorConfigurator.loadTimeInfo();           // load time conf file from SPIFFS
  ObjSensorConfigurator.loadCloudInfo();          // load ObjCloud conf file from SPIFFS
  ObjSensorConfigurator.conFigureSerial(&Serial); // execute serial configuration

  ObjRemote.deviceInfo = &deviceInfo;                      // set the device info reference to the remote manager
  ObjRemote.logFileList = &fileLists;                      // set the log file list reference to the remote manager
  ObjRemote.setNetManager(&ObjWebUI);                      // set the webUI object to the remote manager
  ObjRemote.configurationManager = &ObjSensorConfigurator; // set the configuration manager to the remote manager
  ObjRemote.handledSensorMessage = &sensorDataPacket;      // set the sensor data packet to the remote manager
  ObjRemote.startThread((16 * 1024), 2, 1);                // run remote manager on core 1 (paralel)

  ObjLogger.init();                                        // init logger object
  ObjLogger.configurationManager = &ObjSensorConfigurator; // set the configuration manager to the logger
  ObjLogger.handledLoggingMessage = &sensorDataPacket;     // set the sensor data packet to the logger
  ObjLogger.loggingFlag = &readyToLog;                     // set the logging flag to the logger
  ObjLogger.bufferStoreFlag = &writeBufferFlag;            // set the buffer store flag to the logger
  ObjLogger.startThread(4096, 3, 1);                       // run logger on core 1 (paralel)

  ObjMBInstrument.init(&Serial2); // init the modbus instrument on Serial2

  ObjCloud.setHost(ObjSensorConfigurator.getCloudHost().c_str()); // set the ObjCloud host address
  ObjCloud.begin(ObjSensorConfigurator.getCloudToken().c_str());  // asign token with saved token from configuration

  ObjScheduler.microSD = ObjLogger.microSD; // set the reference CSV logger to the scheduler

  bool initTime = true; // set the initial time update flag to true,for RTC initial

  try
  {
    ObjSensorConfigurator.checkTimeUpdate(&initTime);            // check for time update to sync RTC
    ObjInstrumentManager.initAnalog(0x48, 0x49, GAIN_TWOTHIRDS); // init ADC
  }
  catch (const std::exception &e)
  {
  }

  ObjCloud.reqWorkSpace();                                           // request workspaceID using the token
  ObjCloud.sensorConfiguration = &ObjSensorConfigurator._jsonString; // set the sensor configuration to the ObjCloud

  // I2C Scanner start
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

// end of I2C scanner

void checkReset(uint16_t *tCur, uint16_t *tLast, byte *flag)
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

void loop() // this loop runs on Core1 by default
{
testTimer:
  ObjScheduler.registerCount = &ObjSensorConfigurator.modbusRegistersCount; // set the register count to the scheduler
  ObjScheduler.registers = ObjSensorConfigurator.modbusHREGS;               // set the holding registers to the scheduler

unlicensed:
  sensorDataPacket = ObjSensorConfigurator.getSensorsValue(ObjInstrumentManager, ObjMBInstrument); // sensor data packet which will be send to the ObjCloud
  ObjWebUI.globalMessage = &sensorDataPacket;                                                      // set the broadcast message for handling sensor value viewer
  ObjSensorConfigurator.checkUpdate(&ObjWebUI.sensorUpdated);                                      // check for update if there any changes on the sensor data conf
  ObjSensorConfigurator.checkSerialUpdate(&ObjWebUI.serialComUpdated, ObjMBInstrument);            // check for update if there any changes on the serial comm conf
  ObjSensorConfigurator.checkSiteUpdate(&ObjWebUI.siteUpdated);                                    // check for update if there any changes on the site conf
  ObjSensorConfigurator.checkTimeUpdate(&ObjWebUI.timeUpdated);                                    // check for update if there any changes on the time conf
  ObjSensorConfigurator.checkCloudUpdate(&ObjWebUI.cloudUpdated, &ObjCloud);                       // check for update if there any changes on the ObjCloud conf if update occurs then update the ObjCloud setup
  ObjSensorConfigurator.checkRTCUpdate(&ObjWebUI.RTCUpdated, &ObjWebUI);                           // check for update if there any changes on the RTC Configuration update

  uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes

  if (freeHeap / 1024.0 < 95)
  {
    ESP.restart(); // restart if free heap below 95kB
  }

  Serial.print("Free Heap: ");
  Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
  Serial.println(" KB");

  vTaskDelay(3000 / portTICK_PERIOD_MS); // delay

  if (!ObjLicensing.checkLicense()) // check for license validity, (for dummy condition it set to true[verified])
  {
    goto unlicensed;
  }

  try
  {

    ObjScheduler.checkRegReset(&ObjSensorConfigurator, ObjMBInstrument._modbusInstance);
    ObjScheduler.manage(ObjWebUI.globalMessage,
                        &ObjSensorConfigurator,
                        &ObjWebUI,
                        &ObjCloud,
                        &runUpTimeMinute,
                        &minuteCounter, &readyToLog, &writeBufferFlag);

    // the buffer sending mechanism
    // 1. clone n rows from buffer file to TMP file
    // 2. read the TMP file
    // 3. send TMP data
    // 4. substract the buffer with TMP
    // 5. remove TMP file

    ObjScheduler.sendBuffer(&ObjCloud, &ObjSensorConfigurator);

    fileLists = ObjLogger.microSD->listDir(SD_MMC, "/", 1);
    deviceInfo = getDeviceInfo();
  }
  catch (const std::exception &e)
  {
    Serial.println(e.what());
  }

  if (ObjSensorConfigurator.getHour() == 0 && ObjSensorConfigurator.getMinute() < 2) /// set the NTP sync every midnight
  {
    try
    {
      ObjSensorConfigurator.RTCSync(&RTCSynced);
    }
    catch (const std::exception &e)
    {
      Serial.println(e.what());
    }
  }
  else
  {
    RTCSynced = 0; // reset RTC sync flag
  }
}