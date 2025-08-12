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
        serialTool::run,    // Function
        "Task Serial Tool", // Name
        stackSize,          // Stack size in words (not bytes)
        this,               // Task input parameter
        priority,           // Task priority
        &taskHandle,        // Task handle
        core                // Core ID
    );
}

// Static task entry point
void serialTool::run(void *parameter)
{
    unsigned long t0 = millis();
    serialTool *serComm = static_cast<serialTool *>(parameter);

    while (true)
    {
        Serial.println(serComm->license->getKFactor(0));
        if (millis() - t0 > 2000)
        {
            serComm->streamAIRead(); // stream AI read data
            t0 = millis();
        }
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
                String cmd = serComm->parse(&serComm->RXBufferData);
                String value = "";

                if (cmd == "readinfo") // cmd read serial number
                {
                    /* code */
                    if (serComm->deviceInformation != nullptr && serComm->deviceInformation->length() > 0) // check if device information is set
                    {
                        serComm->hostSerial->print("$");
                        serComm->hostSerial->print(*serComm->deviceInformation);
                        serComm->hostSerial->println("?"); // send the device information
                    }
                    else
                    {
                        serComm->hostSerial->println("{\"error\":\"Device information not set\"}");
                    }
                }
                else if (cmd == "setsn") // set device sn
                {
                    /* code */
                }
                else if (cmd == "setid") // set device id
                {
                    /* code */
                }
                else if (cmd == "getkf0") // set ch0 kfactor
                {
                    serComm->hostSerial->print("$getkf0$");
                    serComm->hostSerial->print("{\"kfactor\":");
                    serComm->hostSerial->print(serComm->license->getKFactor(0).toFloat(), 5);
                    serComm->hostSerial->print("}");
                    serComm->hostSerial->println("?"); // send the kfactor for channel 0
                }
                else if (cmd == "getkf1") // set ch1 kfactor
                {
                    serComm->hostSerial->print("$getkf1$");
                    serComm->hostSerial->print("{\"kfactor\":");
                    serComm->hostSerial->print(serComm->license->getKFactor(1).toFloat(), 5);
                    serComm->hostSerial->print("}");
                    serComm->hostSerial->println("?"); // send the kfactor for channel 1
                }
                else if (cmd == "getkf2") // set ch2 kfactor
                {
                    serComm->hostSerial->print("$getkf2$");
                    serComm->hostSerial->print("{\"kfactor\":");
                    serComm->hostSerial->print(serComm->license->getKFactor(2).toFloat(), 5);
                    serComm->hostSerial->print("}");
                    serComm->hostSerial->println("?"); // send the kfactor for channel 2
                }
                else if (cmd == "getkf3") // set ch3 kfactor
                {
                    serComm->hostSerial->print("$getkf3$");
                    serComm->hostSerial->print("{\"kfactor\":");
                    serComm->hostSerial->print(serComm->license->getKFactor(3).toFloat(), 5);
                    serComm->hostSerial->print("}");
                    serComm->hostSerial->println("?"); // send the kfactor for channel 3
                }
                else if (cmd == "airead") // read AI data
                {
                    serComm->streamAIRead(); // stream AI read data
                }
                else if (cmd == "getlicense") // get license
                {
                    serComm->hostSerial->print("$getlicense$");
                    serComm->hostSerial->print(serComm->license->getLicense());
                    serComm->hostSerial->println("?"); // send the license
                }
                else // exception
                {
                    /* code */
                }

                Serial.println("==========================================");
                serComm->RXBufferData = "";
            }
        }
        Serial.print("RX Buffer Contents:");
        Serial.println(serComm->hostSerial->available());
        serComm->hostSerial->print(*serComm->AIReadData);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

String serialTool::parse(String *command)
{
    String cmdIn = *command;
    cmdIn.replace("$", ""); // remote start byte
    cmdIn.replace("?", ""); // remove stop byte
    Serial.println("command:");
    Serial.println(cmdIn);
    return cmdIn;
}
void serialTool::streamAIRead()
{
    *AIReadData = "{\"ch0\":" + String(aiRawData[0], 5) +
                  ",\"ch1\":" + String(aiRawData[1], 5) +
                  ",\"ch2\":" + String(aiRawData[2], 5) +
                  ",\"ch3\":" + String(aiRawData[3], 5) +
                  ",\"free_heap\":" + String(ESP.getFreeHeap() / 1024.0) +
                  "}";
    hostSerial->print("$airead$");
    hostSerial->print(*AIReadData);
    hostSerial->println("?"); // send the device information
    *AIReadData = "";         // clear the AI read data after sending
}
