#include <TMRLicenseManager.h>
TMRLicenseManager::TMRLicenseManager()
{
    EEPROM.begin(1024);
}
String TMRLicenseManager::decrypt(String enc)
{
    String decrypted = "";
    uint16_t shift = 0;
    uint16_t index = 0;
    for (uint16_t i = 0; i < enc.length(); i++)
    {
        shift = i + 1;
        index = 0;
        for (uint16_t j = 0; j < 92; j++)
        {
            if (enc[i] == lookup[j])
            {
                index = j;
                break;
            }
        }
        decrypted += String(lookup[index - shift]);
    }

    return decrypted;
}

byte TMRLicenseManager::checkLicense()
{
    byte status = 1;
    return status;
}