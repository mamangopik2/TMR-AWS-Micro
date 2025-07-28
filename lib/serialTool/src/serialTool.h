#if !defined(serial_tool_h)
#define serial_tool_h
#include <HardwareSerial.h>

class serialTool
{
private:
    static void run(void *parameter);
    uint32_t bufferSize = 512;

public:
    String RXBufferData;
    String remainingRXBuffer;
    String command;
    HardwareSerial *hostSerial;
    String getRawSerial();
    uint8_t CRCCheck(String command);
    void setRxBufferSize(uint32_t bufferSizee);
    void setBatch();
    void startThread(uint32_t stackSize = 4096,
                     UBaseType_t priority = 1,
                     BaseType_t core = 1);
};

#endif // serial_tool_h
