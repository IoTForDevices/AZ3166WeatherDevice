#include "Arduino.h"
#include "Sensor.h"

DevI2C *ext_i2c;
HTS221Sensor *ht_sensor;
LPS22HBSensor *lp_sensor;

void SetupSensors()
{
    ext_i2c = new DevI2C(D14, D15);
    
    ht_sensor = new HTS221Sensor(*ext_i2c);
    ht_sensor->init(NULL);
    ht_sensor->reset();

    lp_sensor = new LPS22HBSensor(*ext_i2c);
    lp_sensor->init(NULL);
}

float Round(float var) 
{ 
    float value = (int)(var * 10 + .5); 
    return (float)value / 10; 
} 

float ReadHumidity()
{
    float h = 0;
    bool valueRead = false;

    ht_sensor->enable();
    while (! valueRead) {
        valueRead = ht_sensor->getHumidity(&h) == 0;
        if (! valueRead) {
            ht_sensor->reset();
        }
    }
    ht_sensor->disable();
    ht_sensor->reset();

    return Round(h);
}

float ReadTemperature()
{
    float t = 0;
    bool valueRead = false;

    ht_sensor->enable();
    while (! valueRead) {
        valueRead = ht_sensor->getTemperature(&t) == 0;
        if (! valueRead != 0) {
            ht_sensor->reset();
        }
    }
    ht_sensor->disable();
    ht_sensor->reset();

    return Round(t);
}

float ReadPressure()
{
    float p = 0;
    bool valueRead = false;

    while (! valueRead) {
        valueRead = lp_sensor->getPressure(&p) == 0;
    }

    return Round(p);
}

bool IsButtonClicked(unsigned char ulPin)
{
    pinMode(ulPin, INPUT);
    int buttonState = digitalRead(ulPin);
    if(buttonState == LOW)
    {
        return true;
    }
    return false;
}
