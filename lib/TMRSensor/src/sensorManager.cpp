#include "TMRSensor.h"
sensorManager::sensorManager()
{
}

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

String sensorManager::readModbusKF(String tagName1, modbusSensor modbus, uint16_t deviceID, uint16_t dataType,
                                   uint8_t regType, uint16_t regAddr, uint16_t offsett, bool bigEndian, float kFactor)
{
    String uncalibrated = "0";
    String calibrated = "0";

    switch (dataType)
    {
    case MODBUS_UINT16:
    {
        uint16_t val = (regType == IREG)
                           ? modbus.readUnsignedWord(deviceID, IREG, regAddr)
                           : modbus.readUnsignedWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((float)val * kFactor);
        break;
    }
    case MODBUS_INT16:
    {
        int16_t val = (regType == IREG)
                          ? modbus.readSingleWord(deviceID, IREG, regAddr)
                          : modbus.readSingleWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((float)val * kFactor);
        break;
    }
    case MODBUS_UINT32:
    {
        uint32_t val = (regType == IREG)
                           ? modbus.readUnsignedInteger(deviceID, IREG, regAddr, bigEndian)
                           : modbus.readUnsignedInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((float)val * kFactor);
        break;
    }
    case MODBUS_INT32:
    {
        int32_t val = (regType == IREG)
                          ? modbus.readInteger(deviceID, IREG, regAddr, bigEndian)
                          : modbus.readInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((float)val * kFactor);
        break;
    }
    case MODBUS_FLOAT:
    {
        float val = (regType == IREG)
                        ? modbus.readFloat(deviceID, IREG, regAddr, bigEndian)
                        : modbus.readFloat(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String(val * kFactor, 4);
        break;
    }
    case MODBUS_DOUBLE:
    {
        double val = (regType == IREG)
                         ? modbus.readDouble(deviceID, IREG, regAddr, bigEndian)
                         : modbus.readDouble(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String(val * kFactor, 4);
        break;
    }
    default:
    {
        uint16_t val = modbus.readUnsignedWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((float)val * kFactor);
        break;
    }
    }

    String returnVal = "{\"tag_name\":\"" + tagName1 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       uncalibrated +
                       ",\"scaled\":" + calibrated + "}}";
    return returnVal;
}

String sensorManager::readModbus(String tagName2, modbusSensor modbus, uint32_t deviceID, uint8_t dataType, uint16_t regType, uint16_t regAddr, uint8_t offsett, bool bigEndian, float sensitivity)
{
    String uncalibrated = "0";
    String calibrated = "0";

    switch (dataType)
    {
    case MODBUS_UINT16:
    {
        uint16_t val = (regType == IREG)
                           ? modbus.readUnsignedWord(deviceID, IREG, regAddr)
                           : modbus.readUnsignedWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((float)val / sensitivity);
        break;
    }
    case MODBUS_INT16:
    {
        int16_t val = (regType == IREG)
                          ? modbus.readSingleWord(deviceID, IREG, regAddr)
                          : modbus.readSingleWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((float)val / sensitivity);
        break;
    }
    case MODBUS_UINT32:
    {
        uint32_t val = (regType == IREG)
                           ? modbus.readUnsignedInteger(deviceID, IREG, regAddr, bigEndian)
                           : modbus.readUnsignedInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((float)val / sensitivity);
        break;
    }
    case MODBUS_INT32:
    {
        int32_t val = (regType == IREG)
                          ? modbus.readInteger(deviceID, IREG, regAddr, bigEndian)
                          : modbus.readInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((float)val / sensitivity);
        break;
    }
    case MODBUS_FLOAT:
    {
        float val = (regType == IREG)
                        ? modbus.readFloat(deviceID, IREG, regAddr, bigEndian)
                        : modbus.readFloat(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String(val / sensitivity, 4);
        break;
    }
    case MODBUS_DOUBLE:
    {
        double val = (regType == IREG)
                         ? modbus.readDouble(deviceID, IREG, regAddr, bigEndian)
                         : modbus.readDouble(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String(val / sensitivity, 4);
        break;
    }
    default:
    {
        uint16_t val = modbus.readUnsignedWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((float)val / sensitivity);
        break;
    }
    }

    String returnVal = "{\"tag_name\":\"" + tagName2 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       uncalibrated +
                       ",\"scaled\":" + calibrated + "}}";
    return returnVal;
}

String sensorManager::readModbus(String tagName3, modbusSensor modbus, uint16_t deviceID, uint16_t dataType, uint8_t regType, uint16_t regAddr, uint32_t offsett, bool bigEndian, float readoutMin, float readoutMax, float actualMin, float actualMax)
{
    String uncalibrated = "0";
    String calibrated = "0";

    long rMin = (long)readoutMin;
    long rMax = (long)readoutMax;
    long aMin = (long)actualMin;
    long aMax = (long)actualMax;

    switch (dataType)
    {
    case MODBUS_UINT16:
    {
        uint16_t val = modbus.readUnsignedWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        long fmaped = fmap((long)val, rMin, rMax, aMin, aMax);
        calibrated = String(fmaped);
        break;
    }
    case MODBUS_INT16:
    {
        int16_t val = modbus.readSingleWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        long fmaped = fmap((long)val, rMin, rMax, aMin, aMax);
        calibrated = String(fmaped);
        break;
    }
    case MODBUS_UINT32:
    {
        uint32_t val = modbus.readUnsignedInteger(deviceID, HREG, regAddr, true);
        uncalibrated = String(val);
        long fmaped = fmap((long)val, rMin, rMax, aMin, aMax);
        calibrated = String(fmaped);
        break;
    }
    case MODBUS_INT32:
    {
        int32_t val = modbus.readInteger(deviceID, HREG, regAddr, true);
        uncalibrated = String(val);
        long fmaped = fmap((long)val, rMin, rMax, aMin, aMax);
        calibrated = String(fmaped);
        break;
    }
    case MODBUS_FLOAT:
    {
        float val = modbus.readFloat(deviceID, HREG, regAddr, true);
        uncalibrated = String(val, 4);
        long fmaped = fmap((long)val, rMin, rMax, aMin, aMax);
        calibrated = String(fmaped);
        break;
    }
    case MODBUS_DOUBLE:
    {
        double val = modbus.readDouble(deviceID, HREG, regAddr, true);
        uncalibrated = String(val, 4);
        long fmaped = fmap((long)val, rMin, rMax, aMin, aMax);
        calibrated = String(fmaped);
        break;
    }
    default:
    {
        uint16_t val = modbus.readUnsignedWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        long fmaped = fmap((long)val, rMin, rMax, aMin, aMax);
        calibrated = String(fmaped);
        break;
    }
    }

    String returnVal = "{\"tag_name\":\"" + tagName3 + "\"," +
                       "\"value\":{\"unscaled\":" + uncalibrated +
                       ",\"scaled\":" + calibrated + "}}";

    return returnVal;
}

void sensorManager::initAnalog(adsGain_t gain)
{
    Wire.begin(23, 22);
    _ADCInterface = new Adafruit_ADS1015;
    _ADCInterface->setGain(gain);
    _ADCInterface->begin();
}
void sensorManager::initAnalog(uint8_t address, adsGain_t gain)
{
    Wire.begin(23, 22);
    _ADCInterface = new Adafruit_ADS1015;
    _ADCInterface->setGain(gain);
    _ADCInterface->begin();
}

String sensorManager::readAnalog_KF(String tagName1, uint8_t channel, float kFactor)
{
    float sensor_mV;
    float calibrated;
    int16_t results;
    /* Be sure to update this value based on the IC and the gain settings! */
    float multiplier = 0.125;
    switch (channel)
    {
    case 1:
        // CH1
        results = _ADCInterface->readADC_Differential_0_1();
        sensor_mV = _ADCInterface->computeVolts(results);
        break;
    case 2:
        // CH2
        results = _ADCInterface->readADC_Differential_2_3();
        sensor_mV = _ADCInterface->computeVolts(results);
        break;
    }

    calibrated = sensor_mV * kFactor;

    String returnVal = "{\"tag_name\":\"" + tagName1 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       sensor_mV +
                       ",\"scaled\":" + calibrated + "}}";
    return returnVal;
}
String sensorManager::readAnalog_S(String tagName2, uint8_t channel, float sensitivity)
{
    float sensor_mV;
    float calibrated;
    int16_t results;
    /* Be sure to update this value based on the IC and the gain settings! */
    float multiplier = 0.125;
    switch (channel)
    {
    case 1:
        // CH1
        results = _ADCInterface->readADC_Differential_0_1();
        sensor_mV = _ADCInterface->computeVolts(results);
        break;
    case 2:
        // CH2
        results = _ADCInterface->readADC_Differential_2_3();
        sensor_mV = _ADCInterface->computeVolts(results);
        break;
    }

    calibrated = sensor_mV / sensitivity;

    String returnVal = "{\"tag_name\":\"" + tagName2 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       sensor_mV +
                       ",\"scaled\":" + calibrated + "}}";
    return returnVal;
}
String sensorManager::readAnalog_MAP(String tagName2, uint8_t channel, float readoutMin, float readoutMax, float actualMin, float actualMax)
{
    float sensor_mV;
    float calibrated;
    int16_t results;
    /* Be sure to update this value based on the IC and the gain settings! */
    float multiplier = 0.125;
    switch (channel)
    {
    case 1:
        // CH1
        results = _ADCInterface->readADC_Differential_0_1();
        sensor_mV = _ADCInterface->computeVolts(results);
        break;
    case 2:
        // CH2
        results = _ADCInterface->readADC_Differential_2_3();
        sensor_mV = _ADCInterface->computeVolts(results);
        break;
    }

    calibrated = fmap(sensor_mV, readoutMin, readoutMax, actualMax, actualMax);

    String returnVal = "{\"tag_name\":\"" + tagName2 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       sensor_mV +
                       ",\"scaled\":" + calibrated + "}}";
    return returnVal;
}

String sensorManager::readDigital(String tagName, uint8_t channel)
{
    bool logic;
    switch (channel)
    {
    case 1:
        // CH1
        logic = digitalRead(DI1);
        break;
    case 2:
        // CH2
        logic = digitalRead(DI2);
        break;
    case 3:
        // CH3
        logic = digitalRead(DI3);
        break;
    case 4:
        // CH4
        logic = digitalRead(DI4);
        break;
    }

    String returnVal = "{\"tag_name\":\"" + tagName + "\","
                                                      "\"value\":{\"unscaled\":" +
                       logic +
                       ",\"scaled\":" + logic + "}}";
    return returnVal;
}