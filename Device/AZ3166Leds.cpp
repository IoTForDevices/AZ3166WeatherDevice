#line 1 "AZ3166Leds.cpp"
#include "Arduino.h"
#include "RGB_LED.h"
#include "AzureIotHub.h"

#include "IoTHubMessageHandling.h"
#include "config.h"
#include "Utils.h"
#include "DebugZones.h"

static RGB_LED rgbLed;
static uint64_t changeDisplayTrigger = 0;
static char displayBuffer[AZ3166_DISPLAY_SIZE];

/**************************************************************************************************************
 * ShowTelemetryData
 * 
 * Show Humidity, Temperature and Pressure readings on the on-board display.
 **************************************************************************************************************/
void ShowTelemetryData(float temperature, float humidity, float pressure, bool bShowData, bool showHumidity, bool showPressure)
{
    char szSensor[ULONG_64_MAX_DIGITS+1];
    char szTelem[ULONG_64_MAX_DIGITS+1];

    DEBUGMSG_FUNC_IN("(%.1f, %.1f, %.1f, %s, %s, %s)", temperature, humidity, pressure, ShowBool(bShowData), ShowBool(showHumidity), ShowBool(showPressure));

    if (bShowData) {
        snprintf(displayBuffer, AZ3166_DISPLAY_SIZE, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
            f2s(temperature, 1), showHumidity ? f2s(humidity,1) : "-", showPressure ? f2s(pressure,1) : "-");
            Screen.print(displayBuffer);
    } else {
        Screen.clean();
    }
    DEBUGMSG_FUNC_OUT("");
}

/**************************************************************************************************************
 * BlinkLED
 * 
 * Turn on the LED for half a second with a not too bright red color.
 **************************************************************************************************************/
void BlinkLED()
{
    DEBUGMSG_FUNC_IN("()");
    rgbLed.turnOff();
    rgbLed.setColor(AZ3166_RGB_LED_BRIGHTNESS, 0, 0);
    delay(500);
    rgbLed.turnOff();
    DEBUGMSG_FUNC_OUT("");
}

/**************************************************************************************************************
 * BlinkSendConfirmation
 * 
 * Turn on the LED for half a second with a not too bright blue color.
 **************************************************************************************************************/
void BlinkSendConfirmation()
{
    DEBUGMSG_FUNC_IN("()");
    rgbLed.turnOff();
    rgbLed.setColor(0, 0, AZ3166_RGB_LED_BRIGHTNESS);
    delay(500);
    rgbLed.turnOff();
    DEBUGMSG_FUNC_OUT("");
}
