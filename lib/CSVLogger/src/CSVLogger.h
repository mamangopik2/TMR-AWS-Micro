#if !defined(CSVLOGGER_H)
#define CSVLOGGER_H
#include <SDStorage.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <TMRSensor.h>

class CSVLogger
{
private:
    static void run(void *parameter);

public:
    SDStorage *microSD = new SDStorage();
    configReader *configurationManager;
    String *handledLoggingMessage;
    uint8_t *loggingFlag;
    unsigned long log_interval;

    void init();
    void startThread(uint32_t stackSize = 4096,
                     UBaseType_t priority = 1,
                     BaseType_t core = 1);
    bool logJsonArray(String filename, String timestampISO, String data);
    bool setLogInterval(unsigned long interval);
    unsigned long getLogInterval();
    String readLogFile(String filename);
};

#endif // CSVLOGGER_H
