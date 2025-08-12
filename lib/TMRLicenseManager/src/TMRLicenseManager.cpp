#include <TMRLicenseManager.h>
// Serial.print("Original:");
// Serial.println("00:1A:2B:3C:4D:5E");
// Serial.print("encrypted:");
// Serial.println("GH=bH@fJ^$N{En~.`");
// Serial.print("decrypted:");
// Serial.println(licensing.decrypt("GH=bH@fJ^$N{En~.`"));

TMRLicenseManager::TMRLicenseManager()
{
}
void TMRLicenseManager::begin()
{
    Serial.println("TMRLicenseManager begin");
    Serial.print("KF CH0: ");
    Serial.println(EEPROM.readFloat(ADDR_KFCH0));
    Serial.print("KF CH1: ");
    Serial.println(EEPROM.readFloat(ADDR_KFCH1));
    Serial.print("KF CH2: ");
    Serial.println(EEPROM.readFloat(ADDR_KFCH2));
    Serial.print("KF CH3: ");
    Serial.println(EEPROM.readFloat(ADDR_KFCH3));

    if (EEPROM.readFloat(ADDR_KFCH0) <= 0 || isnan(EEPROM.readFloat(ADDR_KFCH0)))
    {
        setKFactor(0, 1.0); // Set default KFactor for channel 0
    }
    if (EEPROM.readFloat(ADDR_KFCH1) <= 0 || isnan(EEPROM.readFloat(ADDR_KFCH1)))
    {
        setKFactor(1, 1.0); // Set default KFactor for channel 1
    }
    {
        setKFactor(1, 1.0); // Set default KFactor for channel 1
    }
    if (EEPROM.readFloat(ADDR_KFCH2) <= 0 || isnan(EEPROM.readFloat(ADDR_KFCH2)))
    {
        setKFactor(2, 1.0); // Set default KFactor for channel 2
    }
    {
        setKFactor(2, 1.0); // Set default KFactor for channel 2
    }
    if (EEPROM.readFloat(ADDR_KFCH3) <= 0 || isnan(EEPROM.readFloat(ADDR_KFCH3)))
    {
        setKFactor(3, 1.0); // Set default KFactor for channel 3
    }
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

String TMRLicenseManager::getSerialNumber()
{
    String sn = "";
    sn = EEPROM.readString(ADDR_SN);
    return sn;
}
String TMRLicenseManager::getDeviceId()
{
    String deviceId = "";
    deviceId = EEPROM.readString(ADDR_DEVICE_ID);
    return deviceId;
}
String TMRLicenseManager::getLicense()
{
    String license = "";
    license = EEPROM.readString(ADDR_LICENSE);
    return license;
}
String TMRLicenseManager::getKFactor(uint8_t channel)
{
    float kfactor = 0.0;
    switch (channel)
    {
    case 0:
        kfactor = EEPROM.readFloat(ADDR_KFCH0);
        break;
    case 1:
        kfactor = EEPROM.readFloat(ADDR_KFCH1);
        break;
    case 2:
        kfactor = EEPROM.readFloat(ADDR_KFCH2);
        break;
    case 3:
        kfactor = EEPROM.readFloat(ADDR_KFCH3);
        break;
    default:
        break;
    }
    return String(kfactor, 5); // return kfactor as string with 5 decimal places
}
void TMRLicenseManager::setKFactor(uint8_t channel, float kfactor)
{
    switch (channel)
    {
    case 0:
        EEPROM.writeFloat(ADDR_KFCH0, kfactor);
        EEPROM.commit(); // commit changes to EEPROM
        break;
    case 1:
        EEPROM.writeFloat(ADDR_KFCH1, kfactor);
        EEPROM.commit(); // commit changes to EEPROM
        break;
    case 2:
        EEPROM.writeFloat(ADDR_KFCH2, kfactor);
        EEPROM.commit(); // commit changes to EEPROM
        break;
    case 3:
        EEPROM.writeFloat(ADDR_KFCH3, kfactor);
        EEPROM.commit(); // commit changes to EEPROM
        break;
    default:
        break;
    }

    EEPROM.commit(); // commit changes to EEPROM
}
void TMRLicenseManager::setSerialNumber(String serial)
{
    EEPROM.writeString(ADDR_SN, serial);
    EEPROM.commit(); // commit changes to EEPROM
}
void TMRLicenseManager::setDeviceId(String deviceId)
{
    EEPROM.writeString(ADDR_DEVICE_ID, deviceId);
    EEPROM.commit(); // commit changes to EEPROM
}
void TMRLicenseManager::setLicense(String license)
{
    EEPROM.writeString(ADDR_LICENSE, license);
    EEPROM.commit(); // commit changes to EEPROM
}