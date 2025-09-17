#include "TMRSensor.h"
modbusSensor::modbusSensor()
{
    _modbusInstance = new ModbusRTU;
}

void modbusSensor::init(HardwareSerial *modbusPort)
{
    _modbusPort = modbusPort;
    _modbusInstance->begin(modbusPort);
    _modbusInstance->master();
}
void modbusSensor::test()
{
    this->_modbusPort->println("Test Port OK!");
}

uint16_t modbusSensor::readUnsignedWord(uint8_t slaveID, uint16_t regType, uint16_t regAddr)
{
    // Serial.print("==========register type: ");
    // Serial.print(regType);
    // Serial.print("===================================\n");
    uint16_t reg = 0;
    if (!_modbusInstance->slave())
    {
        switch (regType)
        {
        case HREG:
            Serial.print("HREG\n");
            _modbusInstance->readHreg(slaveID, regAddr, &reg);
            break;
        case IREG:
            Serial.print("IREG\n");
            _modbusInstance->readIreg(slaveID, regAddr, &reg);
            break;

        default:
            Serial.print("Default HREG\n");
            _modbusInstance->readHreg(slaveID, regAddr, &reg);
        }
    }
    while (_modbusInstance->slave())
    { // Check if transaction is active
        vTaskDelay(1);
        _modbusInstance->task();
    }

    return reg;
}

uint32_t combineWords32(const uint16_t *regs, bool bigEndian)
{
    if (bigEndian)
        return ((uint32_t)regs[0] << 16) | regs[1];
    else
        return ((uint32_t)regs[1] << 16) | regs[0];
}

uint64_t combineWords64(const uint16_t *regs, bool bigEndian)
{
    if (bigEndian)
        return ((uint64_t)regs[0] << 48) | ((uint64_t)regs[1] << 32) | ((uint64_t)regs[2] << 16) | regs[3];
    else
        return ((uint64_t)regs[3] << 48) | ((uint64_t)regs[2] << 32) | ((uint64_t)regs[1] << 16) | regs[0];
}

short modbusSensor::readSingleWord(uint8_t slaveID, uint16_t regType, uint16_t regAddr)
{
    uint16_t reg = readUnsignedWord(slaveID, regType, regAddr);
    return (short)reg;
}

uint32_t modbusSensor::readUnsignedInteger(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian)
{
    uint16_t regs[2] = {0};
    if (!_modbusInstance->slave())
    {
        switch (regType)
        {
        case HREG:
            _modbusInstance->readHreg(slaveID, regAddr, regs, 2);
            break;
        case IREG:
            _modbusInstance->readIreg(slaveID, regAddr, regs, 2);
            break;
        default:
            _modbusInstance->readHreg(slaveID, regAddr, regs, 2);
        }
    }
    while (_modbusInstance->slave())
    {
        vTaskDelay(1);
        _modbusInstance->task();
    }

    return combineWords32(regs, bigEndian);
}

int modbusSensor::readInteger(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian)
{
    uint32_t val = readUnsignedInteger(slaveID, regType, regAddr, bigEndian);
    return (int32_t)val;
}

float modbusSensor::readFloat(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian)
{
    uint16_t regs[2] = {0};
    if (!_modbusInstance->slave())
    {
        switch (regType)
        {
        case HREG:
            _modbusInstance->readHreg(slaveID, regAddr, regs, 2);
            break;
        case IREG:
            _modbusInstance->readIreg(slaveID, regAddr, regs, 2);
            break;
        default:
            _modbusInstance->readHreg(slaveID, regAddr, regs, 2);
        }
    }
    while (_modbusInstance->slave())
    {
        vTaskDelay(1);
        _modbusInstance->task();
    }
    uint32_t combined = 0xffffffff;
    if (bigEndian == true)
    {
        combined = ((uint32_t)regs[1] << 16) | regs[0];
    }
    else
    {
        combined = ((uint32_t)regs[0] << 16) | regs[1];
    }
    float result;
    memcpy(&result, &combined, sizeof(result));
    Serial.print("HBYTE:");
    Serial.print(regs[1], HEX);
    Serial.print("  LBYTE:");
    Serial.print(regs[0], HEX);
    Serial.print("  converted:");
    Serial.println(result);
    return result;

    // uint32_t combined = combineWords32(regs, bigEndian);
    // float value;
    // memcpy(&value, &combined, sizeof(float));
    // return value;
}

double modbusSensor::readDouble(uint8_t slaveID, uint16_t regType, uint16_t regAddr, bool bigEndian)
{
    uint16_t regs[4] = {0};
    if (!_modbusInstance->slave())
    {
        switch (regType)
        {
        case HREG:
            _modbusInstance->readHreg(slaveID, regAddr, regs, 4);
            break;
        case IREG:
            _modbusInstance->readIreg(slaveID, regAddr, regs, 4);
            break;
        default:
            _modbusInstance->readHreg(slaveID, regAddr, regs, 4);
        }
    }
    while (_modbusInstance->slave())
    {
        vTaskDelay(1);
        _modbusInstance->task();
    }

    uint64_t combined = combineWords64(regs, bigEndian);
    double value;
    memcpy(&value, &combined, sizeof(double));
    return value;
}

bool modbusSensor::readCoil(uint8_t slaveID, uint16_t coilAddr)
{
    bool coilState = false;
    if (!_modbusInstance->slave())
    {
        _modbusInstance->readCoil(slaveID, coilAddr, &coilState);
    }

    while (_modbusInstance->slave())
    {
        vTaskDelay(1);
        _modbusInstance->task();
    }

    return coilState;
}

void modbusSensor::writeCoil(uint8_t slaveID, uint16_t coilAddr, bool value)
{
    if (!_modbusInstance->slave())
    {
        _modbusInstance->writeCoil(slaveID, coilAddr, value);
    }

    while (_modbusInstance->slave())
    {
        vTaskDelay(1);
        _modbusInstance->task();
    }
}
