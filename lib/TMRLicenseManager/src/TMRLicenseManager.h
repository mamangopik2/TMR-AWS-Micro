#if !defined(TMRLicenseManager_h)
#define TMRLicenseManager_h
#include <EEPROM.h>
#include <WiFi.h>

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
};

#endif // TMRLicenseManager_h
