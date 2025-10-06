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

uint16_t tcnt0 = 0; // second
uint16_t tcnt1 = 0; // minute
uint16_t tcnt2 = 0; // hour
uint16_t tcnt3 = 0; // day

byte minRstFlag = 0;
byte hourRstFlag = 0;
byte dayRstFlag = 0;
byte monthRstFlag = 0;
byte RTCSynced = 0;

uint32_t lastTimeLookup = 0;

String sensorDataPacket;
uint8_t secondCounter = 0;
uint8_t runUpTimeMinute = 2;
unsigned long minuteCounter = 0;
unsigned long t0 = 0;
unsigned long logger_interval = 0;
uint8_t readyToLog = 0;
uint8_t writeBufferFlag = 0;
String fileLists;
String deviceInfo;
String jsonString;
String AIReadData;

modbusSensor ObjMBInstrument;       // define modbus Instrument
sensorManager ObjInstrumentManager; // define sensor manager
wifiManager ObjWebUI;               // network Interface Manager
configReader ObjSensorConfigurator;
TMRInstrumentWeb ObjCloud;
scheduler ObjScheduler;
TMRemoteMQ ObjRemote;
CSVLogger ObjLogger;
TMRLicenseManager ObjLicensing;
serialTool ObjSystemSerial;

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

  xTaskCreatePinnedToCore(clock, "time scheduler", 1024, NULL, 6, &timeTask, 1);

  ObjWebUI.microSD = ObjLogger.microSD;

  ObjWebUI.logFilelist = &fileLists;
  ObjWebUI.deviceInfo = &deviceInfo;
  ObjWebUI.startThread(4096, 5, 0);

  ObjSensorConfigurator.loadFile();               // load sensors conf
  ObjSensorConfigurator.loadSerialConfigFile();   // load serial comm conf
  ObjSensorConfigurator.loadSiteInfo();           // load site conf
  ObjSensorConfigurator.loadTimeInfo();           // load time conf
  ObjSensorConfigurator.loadCloudInfo();          // load ObjCloud conf
  ObjSensorConfigurator.conFigureSerial(&Serial); // run the configuration

  ObjRemote.deviceInfo = &deviceInfo;
  ObjRemote.logFileList = &fileLists;
  ObjRemote.setNetManager(&ObjWebUI);
  ObjRemote.configurationManager = &ObjSensorConfigurator;
  ObjRemote.handledSensorMessage = &sensorDataPacket;
  ObjRemote.startThread((16 * 1024), 4, 1);

  ObjLogger.init();
  ObjLogger.configurationManager = &ObjSensorConfigurator;
  ObjLogger.handledLoggingMessage = &sensorDataPacket;
  ObjLogger.loggingFlag = &readyToLog;
  ObjLogger.bufferStoreFlag = &writeBufferFlag;
  ObjLogger.startThread(4096, 1, 1);

  ObjMBInstrument.init(&Serial2); // init the modbus instrument

  ObjScheduler.registerCount = &ObjSensorConfigurator.modbusRegistersCount; // set the register count to the scheduler
  ObjScheduler.registers = ObjSensorConfigurator.modbusHREGS;               // set the

  ObjCloud.setHost(ObjSensorConfigurator.getCloudHost().c_str());
  ObjCloud.begin(ObjSensorConfigurator.getCloudToken().c_str());
  bool initTime = true;
  try
  {
    ObjSensorConfigurator.checkTimeUpdate(&initTime);
    ObjInstrumentManager.initAnalog(0x48, GAIN_TWOTHIRDS);
  }
  catch (const std::exception &e)
  {
  }

  ObjCloud.reqWorkSpace();

  ObjCloud.sensorConfiguration = &ObjSensorConfigurator._jsonString;

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
  ObjScheduler.registers = ObjSensorConfigurator.modbusHREGS;               // set the
  currentULPUnixTimestamp = ObjSensorConfigurator.getUnixTime();            // set the RTC unix time to the current unix time

  uint16_t tTemp;

  tTemp = ObjSensorConfigurator.getSecond();
  ObjScheduler.timeComparison(&tTemp, &tcnt0, &minRstFlag);

  tTemp = ObjSensorConfigurator.getMinute();
  ObjScheduler.timeComparison(&tTemp, &tcnt1, &hourRstFlag);

  tTemp = ObjSensorConfigurator.getHour();
  ObjScheduler.timeComparison(&tTemp, &tcnt2, &dayRstFlag);

  tTemp = ObjSensorConfigurator.getDay();
  ObjScheduler.timeComparison(&tTemp, &tcnt3, &monthRstFlag);

  ObjScheduler.resetRegisterByFlag(ObjMBInstrument._modbusInstance, &minRstFlag, ObjSensorConfigurator.modbusRegistersCount);
  ObjScheduler.resetRegisterByFlag(ObjMBInstrument._modbusInstance, &hourRstFlag, ObjSensorConfigurator.modbusRegistersCount);
  ObjScheduler.resetRegisterByFlag(ObjMBInstrument._modbusInstance, &dayRstFlag, ObjSensorConfigurator.modbusRegistersCount);
  ObjScheduler.resetRegisterByFlag(ObjMBInstrument._modbusInstance, &monthRstFlag, ObjSensorConfigurator.modbusRegistersCount);

unlicensed:
  sensorDataPacket = ObjSensorConfigurator.getSensorsValue(ObjInstrumentManager, ObjMBInstrument); // sensor data packet which will be send to the ObjCloud
  ObjWebUI.globalMessage = &sensorDataPacket;                                                      // set the broadcast message for handling sensor value viewer
  ObjSensorConfigurator.checkUpdate(&ObjWebUI.sensorUpdated);                                      // check for update if there any changes on the sensor data conf
  ObjSensorConfigurator.checkSerialUpdate(&ObjWebUI.serialComUpdated, ObjMBInstrument);            // check for update if there any changes on the serial comm conf
  ObjSensorConfigurator.checkSiteUpdate(&ObjWebUI.siteUpdated);                                    // check for update if there any changes on the site conf
  ObjSensorConfigurator.checkTimeUpdate(&ObjWebUI.timeUpdated);                                    // check for update if there any changes on the time conf
  ObjSensorConfigurator.checkCloudUpdate(&ObjWebUI.cloudUpdated, &ObjCloud);                       // check for update if there any changes on the ObjCloud conf if update occurs then update the ObjCloud setup
  ObjSensorConfigurator.checkRTCUpdate(&ObjWebUI.RTCUpdated, &ObjWebUI);                           // check for update if there any changes on the RTC Configuration update

  // uint32_t freeHeap = ESP.getFreeHeap(); // returns bytes
  // Serial.print("Free Heap: ");
  // Serial.print(freeHeap / 1024.0, 2); // convert to KB with 2 decimal places
  // Serial.println(" KB");

  vTaskDelay(3000 / portTICK_PERIOD_MS); // delay

  if (!ObjLicensing.checkLicense())
  {
    goto unlicensed;
  }

  try
  {

    Serial.print(ObjSensorConfigurator.getMinute());
    Serial.print(" % ");
    Serial.print(ObjSensorConfigurator.getCloudInterval());
    Serial.print(" = ");
    Serial.println(ObjSensorConfigurator.getMinute() % ObjSensorConfigurator.getCloudInterval().toInt());

    // the buffer sending mechanism
    // 1. clone n rows from buffer file to TMP file
    // 2. read the TMP file
    // 3. send TMP data
    // 4. substract the buffer with TMP
    // 5. remove TMP file

    // logger.microSD->deleteFile(logger.microSD->card, "/TMRBuffer.csv");
    // logger.microSD->copyFile(logger.microSD->card, "/DATA_LOG_2025-10-06.csv", "/TMRBuffer.csv");
    // Serial.println("Wait 10s to ensure the file is ready");
    // vTaskDelay(10000 / portTICK_PERIOD_MS);

    ObjLogger.microSD->copyFileByLines(ObjLogger.microSD->card, "/TMRBuffer.csv", "/sendTMP.csv", 30);
    String bufferData = "";
    ObjLogger.microSD->readFile(ObjLogger.microSD->card, "/sendTMP.csv", &bufferData);
    if (ObjScheduler.sendBuffer(&ObjCloud, &ObjSensorConfigurator, &bufferData))
    {
      ObjLogger.microSD->substractFile(ObjLogger.microSD->card, "/TMRBuffer.csv", "/sendTMP.csv");
      ObjLogger.microSD->deleteFile(ObjLogger.microSD->card, "/sendTMP.csv");
    }
    else
    {
      ObjLogger.microSD->deleteFile(ObjLogger.microSD->card, "/sendTMP.csv");
    }

    ObjScheduler.manage(ObjWebUI.globalMessage,
                        &ObjSensorConfigurator,
                        &ObjWebUI,
                        &ObjCloud,
                        &runUpTimeMinute,
                        &minuteCounter, &readyToLog, &writeBufferFlag);

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
}