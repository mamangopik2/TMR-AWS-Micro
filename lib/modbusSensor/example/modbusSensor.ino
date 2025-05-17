#include <TMRSensor.h>
modbusSensor mbInstrument();
sensorManager manager;
void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
    mbInstrument.init(&Serial2);
    mbInstrument._modbusInstance->begin(&Serial2);
    mbInstrument._modbusInstance->master();
}
void loop()
{
    String data = manager.readModbus("sensor1", mbInstrument, 1, MODBUS_UINT16, HREG, 0, 0, false, 0.001);
    Serial.println(data);
}