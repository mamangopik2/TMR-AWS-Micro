#include <serialTool.h>

void serialTool::setRxBufferSize(uint32_t bufferSize)
{
    this->bufferSize = bufferSize;
    hostSerial->setRxBufferSize(this->bufferSize);
}

void serialTool::startThread(uint32_t stackSize, UBaseType_t priority, BaseType_t core)
{
    TaskHandle_t taskHandle;
    xTaskCreatePinnedToCore(
        serialTool::run, // Function
        "MyTask",        // Name
        stackSize,       // Stack size in words (not bytes)
        this,            // Task input parameter
        priority,        // Task priority
        &taskHandle,     // Task handle
        core             // Core ID
    );
}

// Static task entry point
void serialTool::run(void *parameter)
{
    serialTool *serComm = static_cast<serialTool *>(parameter);
    while (true)
    {
        if (serComm->hostSerial->available())
        {
            uint32_t availableByteInBuffer = serComm->hostSerial->available();
            for (uint32_t i = 0; i < availableByteInBuffer; i++)
            {
                serComm->RXBufferData += (char)serComm->hostSerial->read();
            }
        }
        else
        {
            if (serComm->RXBufferData.length() > 0)
            {
                Serial.println("==========================================");
                Serial.println(serComm->RXBufferData);
                Serial.println("==========================================");
                serComm->RXBufferData = "";
            }
        }
        Serial.print("RX Buffer Contents:");
        Serial.println(serComm->hostSerial->available());
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}