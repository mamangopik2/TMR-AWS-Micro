#if !defined(TMRLicenseManager_h)
#define TMRLicenseManager_h
#include <EEPROM.h>
#include <WiFi.h>
#define ADDR_KFCH0 0x00
#define ADDR_KFCH1 0x04
#define ADDR_KFCH2 0x08
#define ADDR_KFCH3 0x0C
#define ADDR_SN 0x20
#define ADDR_LICENSE 0x50
#define ADDR_DEVICE_ID 0x80

class TMRLicenseManager
{
private:
    /* data */
public:
    TMRLicenseManager();
    const char lookup[92]{
        'A', 'B', 'C', '0', 'G', 'H', ')', '*',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', '+', '1', '2', 'D',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
        'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
        'y', 'z', '3', '4', '5',
        '6', '7', '8', '9', '!', '"', '#', '$',
        '%', '&', '(', 'E', 'F', ',',
        '-', '.', '/', ':', ';', '<', '=', '>',
        '?', '@', '[', ']', '^', '_', '`',
        '{', '|', '}', '~'};

    String decrypt(String enc);
    byte checkLicense();
    String getSerialNumber();
    String getDeviceId();
    String getLicense();
    String getKFactor(uint8_t channel);
    void setKFactor(uint8_t channel, float kfactor);
    void setSerialNumber(String serial);
    void setDeviceId(String deviceId);
    void setLicense(String license);
    void begin();
};

#endif // TMRLicenseManager_h
