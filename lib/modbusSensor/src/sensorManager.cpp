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
