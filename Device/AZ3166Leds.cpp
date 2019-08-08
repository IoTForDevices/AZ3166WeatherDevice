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
void ShowTelemetryData(uint64_t upTime, uint64_t sensorReads, uint64_t telemetrySend, float temperature, float humidity, float pressure, bool showHumidity, bool showPressure)
{
    bool display1 = (changeDisplayTrigger < (upTime % 300));
    char szUpHours[ULONG_64_MAX_DIGITS+1];
    char szSensor[ULONG_64_MAX_DIGITS+1];
    char szTelem[ULONG_64_MAX_DIGITS+1];

#ifdef LOGGING
    char szUp[ULONG_64_MAX_DIGITS+1];
    char szSR[ULONG_64_MAX_DIGITS+1];
    char szTS[ULONG_64_MAX_DIGITS+1];
    char szDT[ULONG_64_MAX_DIGITS+1];

    Int64ToString(upTime, szUp, ULONG_64_MAX_DIGITS);
    Int64ToString(sensorReads, szSR, ULONG_64_MAX_DIGITS);
    Int64ToString(telemetrySend, szTS, ULONG_64_MAX_DIGITS);
    Int64ToString(changeDisplayTrigger, szDT, ULONG_64_MAX_DIGITS);

    DEBUGMSG_FUNC_IN("(%s, %s, %s, %.1f, %.1f, %.1f, %s, %s)", szUp, szSR, szTS, temperature, humidity, pressure, ShowBool(showHumidity), ShowBool(showPressure));

    DEBUGMSG(ZONE_SENSORDATA, "upTime = %s, sensorReads = %s, telemetrySend = %s", szUp, szSR, szTS);
    DEBUGMSG(ZONE_SENSORDATA, "display1 = %s, changeDisplayTrigger = %s", ShowBool(display1),  szDT);
#endif

    changeDisplayTrigger = (upTime % 300);
    Int64ToString(upTime / 3600, szUpHours, sizeof(szUpHours) - 1);

    if (display1) {
        snprintf(displayBuffer, AZ3166_DISPLAY_SIZE, "UP:%s\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
            szUpHours, f2s(temperature, 1), showHumidity ? f2s(humidity,1) : "-", showPressure ? f2s(pressure,1) : "-");
    } else {
        Int64ToString(sensorReads, szSensor, 15);
        Int64ToString(telemetrySend, szTelem, 15);
        snprintf(displayBuffer, AZ3166_DISPLAY_SIZE, "#S:%s\r\n#T:%s\r\nTemp:%s\r\nH:%s%%-P:%s\r\n",
            szSensor, szTelem, f2s(temperature, 1), showHumidity ? f2s(humidity,1) : "-", showPressure ? f2s(pressure,1) : "-");
    }
    Screen.print(displayBuffer);
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
