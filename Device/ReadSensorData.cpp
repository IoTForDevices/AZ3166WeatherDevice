#include "Arduino.h"
#include "azure_c_shared_utility/xlogging.h"
#include "Sensor.h"
#include "DebugZones.h"

static DevI2C *ext_i2c;
static HTS221Sensor *ht_sensor;
static LPS22HBSensor *lp_sensor;
static LSM6DSLSensor *acc_gyro;

void SetupSensors()
{
    ext_i2c = new DevI2C(D14, D15);
    
    ht_sensor = new HTS221Sensor(*ext_i2c);
    ht_sensor->init(NULL);
    ht_sensor->reset();

    lp_sensor = new LPS22HBSensor(*ext_i2c);
    lp_sensor->init(NULL);

    acc_gyro = new LSM6DSLSensor(*ext_i2c, D4, D5);
    acc_gyro->init(NULL);
    acc_gyro->enableAccelerator();
    acc_gyro->enableGyroscope();
}

double Round(float var) 
{ 
    double value = (int)(var * 10 + .5); 
    return (double)value / 10; 
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

int axes[3];
static bool motionInitialized = false;
static int xReading;
static int yReading;
static int zReading;

bool MotionDetected(int motionSensitivity)
{
    bool hasFailed = false;
    bool motionDetected = false;

    hasFailed = (acc_gyro->getXAxes(axes) != 0);
    
    if (! hasFailed)
    {
        DEBUGMSG(ZONE_MOTIONDETECT, "Accelerator Axes - x: %d, y: %d, z: %d", hasFailed ? 0xFFFF : axes[0], axes[1], axes[2]);
        if (! motionInitialized)
        {
            xReading = axes[0];
            yReading = axes[1];
            zReading = axes[2];
            motionInitialized = true;
        }
        else
        {
            motionDetected = abs(xReading - axes[0]) > motionSensitivity || abs(yReading - axes[1]) > motionSensitivity || abs(zReading - axes[2]) > motionSensitivity;
            DEBUGMSG(ZONE_MOTIONDETECT, "Motion detected: %s", motionDetected ? "true" : "false");
            xReading = axes[0];
            yReading = axes[1];
            zReading = axes[2];
        }
        
    }
    return motionDetected;
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
