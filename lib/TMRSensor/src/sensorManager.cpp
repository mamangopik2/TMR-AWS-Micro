#include "TMRSensor.h"
sensorManager::sensorManager()
{
}

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

String sensorManager::readModbusKF(String EU, String RU, String tagName1, modbusSensor modbus, uint16_t deviceID, uint16_t dataType,
                                   uint8_t regType, uint16_t regAddr, uint16_t offsett, bool bigEndian, float kFactor, float ofset)
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
        calibrated = String((val * kFactor) + ofset, 4);
        break;
    }
    case MODBUS_INT16:
    {
        int16_t val = (regType == IREG)
                          ? modbus.readSingleWord(deviceID, IREG, regAddr)
                          : modbus.readSingleWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((val * kFactor) + ofset, 4);
        break;
    }
    case MODBUS_UINT32:
    {
        uint32_t val = (regType == IREG)
                           ? modbus.readUnsignedInteger(deviceID, IREG, regAddr, bigEndian)
                           : modbus.readUnsignedInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((val * kFactor) + ofset, 4);
        break;
    }
    case MODBUS_INT32:
    {
        int32_t val = (regType == IREG)
                          ? modbus.readInteger(deviceID, IREG, regAddr, bigEndian)
                          : modbus.readInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((val * kFactor) + ofset, 4);
        break;
    }
    case MODBUS_FLOAT:
    {
        float val = (regType == IREG)
                        ? modbus.readFloat(deviceID, IREG, regAddr, bigEndian)
                        : modbus.readFloat(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String((val * kFactor) + ofset, 4);
        break;
    }
    case MODBUS_DOUBLE:
    {
        double val = (regType == IREG)
                         ? modbus.readDouble(deviceID, IREG, regAddr, bigEndian)
                         : modbus.readDouble(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String((val * kFactor) + ofset, 4);
        break;
    }
    default:
    {
        uint16_t val = modbus.readUnsignedWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((float)(val * kFactor) + ofset);
        break;
    }
    }

    String returnVal = "{\"tag_name\":\"" + tagName1 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       uncalibrated +
                       ",\"scaled\":" + calibrated + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
    return returnVal;
}

String sensorManager::readModbus(String EU, String RU, String tagName2, modbusSensor modbus, uint32_t deviceID, uint8_t dataType, uint16_t regType, uint16_t regAddr, uint8_t offsett, bool bigEndian, float sensitivity, float ofset)
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
        calibrated = String((val / sensitivity) + ofset, 4);
        break;
    }
    case MODBUS_INT16:
    {
        int16_t val = (regType == IREG)
                          ? modbus.readSingleWord(deviceID, IREG, regAddr)
                          : modbus.readSingleWord(deviceID, HREG, regAddr);
        uncalibrated = String(val);
        calibrated = String((val / sensitivity) + ofset, 4);
        break;
    }
    case MODBUS_UINT32:
    {
        uint32_t val = (regType == IREG)
                           ? modbus.readUnsignedInteger(deviceID, IREG, regAddr, bigEndian)
                           : modbus.readUnsignedInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((val / sensitivity) + ofset, 4);
        break;
    }
    case MODBUS_INT32:
    {
        int32_t val = (regType == IREG)
                          ? modbus.readInteger(deviceID, IREG, regAddr, bigEndian)
                          : modbus.readInteger(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val);
        calibrated = String((val / sensitivity) + ofset, 4);
        break;
    }
    case MODBUS_FLOAT:
    {
        float val = (regType == IREG)
                        ? modbus.readFloat(deviceID, IREG, regAddr, bigEndian)
                        : modbus.readFloat(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String((val / sensitivity) + ofset, 4);
        break;
    }
    case MODBUS_DOUBLE:
    {
        double val = (regType == IREG)
                         ? modbus.readDouble(deviceID, IREG, regAddr, bigEndian)
                         : modbus.readDouble(deviceID, HREG, regAddr, bigEndian);
        uncalibrated = String(val, 4);
        calibrated = String((val / sensitivity) + ofset, 4);
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
                       ",\"scaled\":" + calibrated + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
    return returnVal;
}

String sensorManager::readModbus(String EU, String RU, String tagName3, modbusSensor modbus, uint16_t deviceID, uint16_t dataType, uint8_t regType, uint16_t regAddr, uint32_t offsett, bool bigEndian, float readoutMin, float readoutMax, float actualMin, float actualMax)
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
                       ",\"scaled\":" + calibrated + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";

    return returnVal;
}

void sensorManager::initAnalog(adsGain_t gain)
{
    Wire.begin(21, 22);
    _ADCInterface1 = new Adafruit_ADS1015;
    _ADCInterface1->setGain(gain);
    _ADCInterface1->begin();
}
void sensorManager::initAnalog(uint8_t address1, uint8_t address2, adsGain_t gain)
{
    Wire.begin(21, 22);
    _ADCInterface1 = new Adafruit_ADS1015;
    _ADCInterface1->setGain(gain);
    _ADCInterface1->begin(address1, &Wire);

    _ADCInterface2 = new Adafruit_ADS1015;
    _ADCInterface2->setGain(gain);
    _ADCInterface2->begin(address2, &Wire);
}

String sensorManager::readAnalog_KF(String EU, String RU, String tagName1, uint8_t channel, float kFactor, float ofset)
{
    float sensor_mV;
    float calibrated;
    int16_t results;
    switch (channel)
    {
    case 1:
        // CH1
        try
        {
            results = _ADCInterface1->readADC_Differential_0_1();
            sensor_mV = _ADCInterface1->computeVolts(results) * licenseManager->getKFactor(1).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }

        break;
    case 2:
        // CH2
        try
        {
            results = _ADCInterface1->readADC_Differential_2_3();
            sensor_mV = _ADCInterface1->computeVolts(results) * licenseManager->getKFactor(1).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    case 3:
        // CH3
        try
        {
            results = _ADCInterface2->readADC_Differential_0_1();
            sensor_mV = _ADCInterface2->computeVolts(results) * licenseManager->getKFactor(3).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    case 4:
        // CH4
        try
        {
            results = _ADCInterface2->readADC_Differential_2_3();
            sensor_mV = _ADCInterface2->computeVolts(results) * licenseManager->getKFactor(4).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    }
    analogReadData[channel - 1] = sensor_mV; // Store the read value in the analogReadData array

    calibrated = (sensor_mV * kFactor) + ofset;

    String returnVal = "{\"tag_name\":\"" + tagName1 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       sensor_mV +
                       ",\"scaled\":" + String(calibrated, 4) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
    return returnVal;
}
String sensorManager::readAnalog_S(String EU, String RU, String tagName2, uint8_t channel, float sensitivity, float ofset)
{
    float sensor_mV;
    float calibrated;
    int16_t results;
    switch (channel)
    {
    case 1:
        // CH1
        try
        {
            results = _ADCInterface1->readADC_Differential_0_1();
            sensor_mV = _ADCInterface1->computeVolts(results) * licenseManager->getKFactor(1).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }

        break;
    case 2:
        // CH2
        try
        {
            results = _ADCInterface1->readADC_Differential_2_3();
            sensor_mV = _ADCInterface1->computeVolts(results) * licenseManager->getKFactor(1).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    case 3:
        // CH3
        try
        {
            results = _ADCInterface2->readADC_Differential_0_1();
            sensor_mV = _ADCInterface2->computeVolts(results) * licenseManager->getKFactor(3).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    case 4:
        // CH4
        try
        {
            results = _ADCInterface2->readADC_Differential_2_3();
            sensor_mV = _ADCInterface2->computeVolts(results) * licenseManager->getKFactor(4).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    }
    analogReadData[channel - 1] = sensor_mV; // Store the read value in the analogReadData array
    calibrated = (sensor_mV / sensitivity) + ofset;

    String returnVal = "{\"tag_name\":\"" + tagName2 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       sensor_mV +
                       ",\"scaled\":" + String(calibrated, 4) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
    return returnVal;
}
String sensorManager::readAnalog_MAP(String EU, String RU, String tagName2, uint8_t channel, float readoutMin, float readoutMax, float actualMin, float actualMax)
{
    float sensor_mV;
    float calibrated;
    int16_t results;
    switch (channel)
    {
    case 1:
        // CH1
        try
        {
            results = _ADCInterface1->readADC_Differential_0_1();
            sensor_mV = _ADCInterface1->computeVolts(results) * licenseManager->getKFactor(1).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }

        break;
    case 2:
        // CH2
        try
        {
            results = _ADCInterface1->readADC_Differential_2_3();
            sensor_mV = _ADCInterface1->computeVolts(results) * licenseManager->getKFactor(1).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    case 3:
        // CH3
        try
        {
            results = _ADCInterface2->readADC_Differential_0_1();
            sensor_mV = _ADCInterface2->computeVolts(results) * licenseManager->getKFactor(3).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    case 4:
        // CH4
        try
        {
            results = _ADCInterface2->readADC_Differential_2_3();
            sensor_mV = _ADCInterface2->computeVolts(results) * licenseManager->getKFactor(4).toFloat();
        }
        catch (const std::exception &e)
        {
            results = 0;     // Handle the exception by setting results to a default value
            sensor_mV = 0.0; // Set a default value for sensor_mV in case of an error
        }
        break;
    }
    analogReadData[channel - 1] = sensor_mV; // Store the read value in the analogReadData array
    calibrated = fmap(sensor_mV, readoutMin, readoutMax, actualMax, actualMax);

    String returnVal = "{\"tag_name\":\"" + tagName2 + "\","
                                                       "\"value\":{\"unscaled\":" +
                       sensor_mV +
                       ",\"scaled\":" + String(calibrated, 4) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
    return returnVal;
}

String sensorManager::readCoilRegister(String EU, String RU, String tagNameCoil, modbusSensor modbus, uint32_t deviceID, uint16_t regAddr)
{
    bool data = modbus.readCoil(deviceID, regAddr);
    String returnVal = "{\"tag_name\":\"" + tagNameCoil + "\","
                                                          "\"value\":{\"unscaled\":" +
                       String(data) +
                       ",\"scaled\":" + String(data) + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";
    return String(returnVal);
}

String sensorManager::readDigital(String EU, String RU, String tagName, uint8_t channel)
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
                       ",\"scaled\":" + logic + "},\"eng_unit\":\"" + EU + "\",\"raw_unit\":\"" + RU + "\"}";

    return returnVal;
}
void sensorManager::readRawAnalog()
{
    float sensor_mV;
    int16_t results;
    /* Be sure to update this value based on the IC and the gain settings! */
    float multiplier = 0.125;
    results = _ADCInterface1->readADC_Differential_0_1();
    sensor_mV = (_ADCInterface1->computeVolts(results)) * licenseManager->getKFactor(0).toFloat();
    analogReadData[0] = sensor_mV; // Store the read value in the analogReadData array
    results = _ADCInterface1->readADC_Differential_2_3();
    sensor_mV = (_ADCInterface1->computeVolts(results)) * licenseManager->getKFactor(1).toFloat();
    analogReadData[1] = sensor_mV; // Store the read value in the analogReadData array
    results = _ADCInterface2->readADC_Differential_0_1();
    sensor_mV = (_ADCInterface2->computeVolts(results)) * licenseManager->getKFactor(2).toFloat();
    analogReadData[2] = sensor_mV; // Store the read value in the analogReadData array
    results = _ADCInterface2->readADC_Differential_2_3();
    sensor_mV = (_ADCInterface2->computeVolts(results)) * licenseManager->getKFactor(3).toFloat();
    analogReadData[3] = sensor_mV; // Store the read value in the analogReadData array
}
