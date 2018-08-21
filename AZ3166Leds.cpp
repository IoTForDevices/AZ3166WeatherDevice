#include "arduino.h"
#include "RGB_LED.h"

#define RGB_LED_BRIGHTNESS 32
#define DISPLAY_SIZE 128

static RGB_LED rgbLed;
static char displayBuffer[DISPLAY_SIZE];

void ShowTelemetryData(float temperature, float humidity, float pressure)
{
    snprintf(displayBuffer, DISPLAY_SIZE, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",f2s(temperature, 1), f2s(humidity,1), f2s(pressure,1));
    Screen.print(displayBuffer);
}

void BlinkLED()
{
    rgbLed.turnOff();
    rgbLed.setColor(RGB_LED_BRIGHTNESS, 0, 0);
    delay(500);
    rgbLed.turnOff();
}

void BlinkSendConfirmation()
{
    rgbLed.turnOff();
    rgbLed.setColor(0, 0, RGB_LED_BRIGHTNESS);
    delay(500);
    rgbLed.turnOff();
}
