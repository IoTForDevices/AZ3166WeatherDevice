#include "Arduino.h"
#include "RGB_LED.h"
#include "AzureIotHub.h"

#include "IoTHubMessageHandling.h"

#define RGB_LED_BRIGHTNESS 32
#define DISPLAY_SIZE 128

static RGB_LED rgbLed;
static char displayBuffer[DISPLAY_SIZE];

/**************************************************************************************************************
 * ShowTelemetryData
 * 
 * Show Humidity, Temperature and Pressure readings on the on-board display.
 **************************************************************************************************************/
void ShowTelemetryData(float temperature, float humidity, float pressure, bool showHumidity, bool showPressure)
{
    snprintf(displayBuffer, DISPLAY_SIZE, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
        f2s(temperature, 1), showHumidity ? f2s(humidity,1) : "-", showPressure ? f2s(pressure,1) : "-");
    Screen.print(displayBuffer);
}

/**************************************************************************************************************
 * BlinkLED
 * 
 * Turn on the LED for half a second with a not too bright red color.
 **************************************************************************************************************/
void BlinkLED()
{
    rgbLed.turnOff();
    rgbLed.setColor(RGB_LED_BRIGHTNESS, 0, 0);
    delay(500);
    rgbLed.turnOff();
}

/**************************************************************************************************************
 * BlinkSendConfirmation
 * 
 * Turn on the LED for half a second with a not too bright blue color.
 **************************************************************************************************************/
void BlinkSendConfirmation()
{
    rgbLed.turnOff();
    rgbLed.setColor(0, 0, RGB_LED_BRIGHTNESS);
    delay(500);
    rgbLed.turnOff();
}
