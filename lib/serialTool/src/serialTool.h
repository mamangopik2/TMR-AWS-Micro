#if !defined(serial_tool_h)
#define serial_tool_h
#include <HardwareSerial.h>
#include <TMRSensor.h>
#include <TMRLicenseManager.h>

class serialTool
{
private:
    static void run(void *parameter);
    uint32_t bufferSize = 512;

public:
    TMRLicenseManager *license;
    float *aiRawData;
    String RXBufferData;
    String remainingRXBuffer;
    String command;
    String *deviceInformation;
    String *AIReadData;
    HardwareSerial *hostSerial;
    String getRawSerial();
    uint8_t CRCCheck(String command);
    void setRxBufferSize(uint32_t bufferSizee);
    void setBatch();
    void startThread(uint32_t stackSize = 4096,
                     UBaseType_t priority = 1,
                     BaseType_t core = 1);
    String parse(String *command);
    void streamAIRead();
};

#endif // serial_tool_h
