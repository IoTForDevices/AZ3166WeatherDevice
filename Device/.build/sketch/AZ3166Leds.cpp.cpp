#line 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
#line 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
#include "Arduino.h"
#include "RGB_LED.h"
#include "AzureIotHub.h"

#include "IoTHubMessageHandling.h"

#define RGB_LED_BRIGHTNESS 32
#define DISPLAY_SIZE 128

static RGB_LED rgbLed;
static char displayBuffer[DISPLAY_SIZE];

#line 13 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void ShowTelemetryData(float temperature, float humidity, float pressure, bool showHumidity, bool showPressure);
#line 20 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void BlinkLED();
#line 28 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void BlinkSendConfirmation();
#line 55 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length);
#line 61 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
bool InitWifi();
#line 74 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
bool InitIoTHub();
#line 88 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength);
#line 107 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void setup();
#line 145 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void Int64ToString(uint64_t uiValue, char *pszValue);
#line 156 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void loop();
#line 13 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void ShowTelemetryData(float temperature, float humidity, float pressure, bool showHumidity, bool showPressure)
{
    snprintf(displayBuffer, DISPLAY_SIZE, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
        f2s(temperature, 1), showHumidity ? f2s(humidity,1) : "-", showPressure ? f2s(pressure,1) : "-");
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

#line 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
// To get started please visit https://microsoft.github.io/azure-iot-developer-kit/docs/projects/remote-monitoring/?utm_source=ArduinoExtension&utm_medium=ReleaseNote&utm_campaign=VSCode
#include "Arduino.h"
#include "Sensor.h"
#include "AzureIotHub.h"
#include "AZ3166WiFi.h"
#include "DevKitMQTTClient.h"

#include "Telemetry.h"
#include "SystemTime.h"
#include "SystemTickCounter.h"

#include "Config.h"
#include "IoTHubDeviceMethods.h"
#include "IoTHubMessageHandling.h"
#include "ReadSensorData.h"
#include "UpdateFirmwareOTA.h"
#include "DebugZones.h"

#include <inttypes.h>

#ifdef LOGGING
extern "C" DBGPARAM dpCurSettings =
{
    "AZ3166WeatherDevice",
    {
        ZONE0_TEXT, ZONE1_TEXT, ZONE2_TEXT, ZONE3_TEXT,
        ZONE4_TEXT, ZONE5_TEXT, ZONE6_TEXT, ZONE7_TEXT,
        ZONE8_TEXT, ZONE9_TEXT, ZONE10_TEXT, ZONE11_TEXT,
        ZONE12_TEXT, ZONE13_TEXT, ZONE14_TEXT, ZONE15_TEXT
    },
    DEBUGZONES
};
#endif

static bool onReset = false;
static bool onMeasureNow = false;
static bool onFirmwareUpdate = false;

static bool messageSending = true;
static uint64_t send_interval_ms;
static uint64_t measure_interval_ms;
static uint64_t warming_up_interval_ms;
static uint64_t motion_interval_ms;
static uint64_t deviceStartTime = 0;
static int upTime;

static bool nextMeasurementDue;
static bool firstMessageSend = false;
static bool nextMessageDue;
static bool nextMotionEventDue;
static bool suppressMessages;

void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    DEBUGMSG(ZONE_INIT, "TwinCallback - Payload: Length = %d", length)
    ParseTwinMessage(updateState, (char *)payLoad);
}

bool InitWifi()
{
    bool hasWifi = false;

    if (WiFi.begin() == WL_CONNECTED) {
        hasWifi = true;
        Screen.print(1, "Running...");
    } else {
        Screen.print(1, "No Wi-Fi!");
    }
    return hasWifi;
}

bool InitIoTHub()
{
    bool hasIoTHub = DevKitMQTTClient_Init(true);
    if (hasIoTHub) {
        Screen.print(1, "Running....");
    } else {
        Screen.print(1, "No IoTHub!");
    }
    return hasIoTHub;
}

/* IoT Hub Callback functions.
 * NOTE: These functions must be available inside this source file, prior to the Init and Loop methods.
 */
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength)
{
    int result = 200;

    DEBUGMSG(ZONE_INIT, "--> %s(methodName = %s)", FUNC_NAME, methodName);

    if (strcmp(methodName, "Reset") == 0) {
        onReset = HandleReset(response, responseLength);
    } else if (strcmp(methodName, "MeasureNow") == 0) {
        onMeasureNow = HandleMeasureNow(response, responseLength);
    } else if (strcmp(methodName, "FirmwareUpdate") == 0) {
        onFirmwareUpdate = HandleFirmwareUpdate((const char*)payload, length, response, responseLength);
    } else {
        result = 500;
        HandleUnknownMethod(response, responseLength);
    }
    return result;
}

void setup()
{
    // int nLength = strlen(DEVICE_ID);
    // reportedDeviceProperties.pszDeviceModel = (char *)malloc(nLength + 1);
    // if (reportedDeviceProperties.pszDeviceModel == NULL) {
    //     exit(1);
    // }
    // snprintf(reportedDeviceProperties.pszDeviceModel, nLength + 1, "%s", DEVICE_ID);

    // nLength = strlen(DEVICE_LOCATION);
    // reportedDeviceProperties.pszLocation = (char *)malloc(nLength + 1);
    // if (reportedDeviceProperties.pszLocation == NULL) {
    //     exit(1);
    // }
    // snprintf(reportedDeviceProperties.pszLocation, nLength + 1, "%s", DEVICE_LOCATION);

    // nLength = strlen(DEVICE_FIRMWARE_VERSION);
    // reportedDeviceProperties.pszCurrentFwVersion = (char *)malloc(nLength + 1);
    // if (reportedDeviceProperties.pszCurrentFwVersion == NULL) {
    //     exit(1);
    // }
    // snprintf(reportedDeviceProperties.pszCurrentFwVersion, nLength + 1, "%s", DEVICE_FIRMWARE_VERSION);

    if (!InitWifi()) {
        exit(1);
    }
    if (!InitIoTHub()) {
        exit(1);
    }

    DevKitMQTTClient_SetDeviceMethodCallback(&DeviceMethodCallback);
    DevKitMQTTClient_SetDeviceTwinCallback(&TwinCallback);

    SetupSensors();
    send_interval_ms = measure_interval_ms = warming_up_interval_ms = deviceStartTime = motion_interval_ms = SystemTickCounterRead();
}

#ifdef LOGGING
void Int64ToString(uint64_t uiValue, char *pszValue)
{
    const int NUM_DIGITS = log10(uiValue) + 1;
    *(pszValue+NUM_DIGITS) = '\0';

    for (size_t i = NUM_DIGITS; i--; uiValue /= 10) {
        *(pszValue+i) = '0' + (uiValue % 10);
    }
}
#endif

void loop()
{
    if (InitialDeviceTwinDesiredReceived()) {
        uint64_t tickCount = SystemTickCounterRead();
#ifdef LOGGING
        char szTC[40];
        char szMI[40];
        char szSI[40];
        char szMM[40];
        char szWU[40];

        Int64ToString(tickCount, szTC);
        Int64ToString(measure_interval_ms, szMI);
        Int64ToString(send_interval_ms, szSI);
        Int64ToString(motion_interval_ms, szMM);
        Int64ToString(warming_up_interval_ms, szWU);
#endif
        nextMeasurementDue = (tickCount - measure_interval_ms) >= MeasurementInterval();
        DEBUGMSG(ZONE_MAINLOOP, "%s(%d), nextMeasurementDue = %d, tickCount = %s, measure_interval_ms = %s, MeasurementInterval() = %d", FUNC_NAME, __LINE__, nextMeasurementDue, szTC, szMI, MeasurementInterval());
        nextMessageDue = (tickCount - send_interval_ms) >= SendInterval();
        DEBUGMSG(ZONE_MAINLOOP, "%s(%d), nextMessageDue = %d, tickCount = %s, send_interval_ms = %s, SendInterval() = %d", FUNC_NAME, __LINE__, nextMessageDue, szTC, szSI, SendInterval());
        nextMotionEventDue = (tickCount - motion_interval_ms) >= MotionInterval();
        DEBUGMSG(ZONE_MAINLOOP, "%s(%d), nextMotionEventDue = %d, tickCount = %s, motion_interval_ms = %s, MotionInterval() = %d", FUNC_NAME, __LINE__, nextMotionEventDue, szTC, szMM, MotionInterval());

        if (! firstMessageSend) {
            suppressMessages = (tickCount - warming_up_interval_ms) < WarmingUpTime();
            DEBUGMSG(ZONE_MAINLOOP, "%s(%d), suppressMessages = %d, SystemTickCounterRead() = %s, warming_up_interval_ms = %s, WarmingUpTime() = %d", FUNC_NAME, __LINE__, suppressMessages, szTC, szWU, WarmingUpTime());

            if (! suppressMessages) {
                firstMessageSend = true;
                nextMessageDue = true;
            }
        }

        if (MotionDetectionEnabled()) {
            // Check if the device is in motion or not and enable an alarm if so.
            bool motionDetected = MotionDetected(MotionSensitivity());

            if (motionDetected && nextMotionEventDue) {
                char messageEvt[MESSAGE_MAX_LEN];
                CreateEventMsg(messageEvt, MOTION_EVENT);
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
                DevKitMQTTClient_SendEventInstance(message);

                motion_interval_ms = SystemTickCounterRead();
                DEBUGMSG(ZONE_MOTIONDETECT, "%s(%d) - Sending Motion Detected Event - motion_inteval_ms = %d", FUNC_NAME, __LINE__, motion_interval_ms);
            }

            DEBUGMSG(ZONE_MOTIONDETECT, "%s(%d) - Motion Detected = %d, nextMotionEventDue = %d", FUNC_NAME, __LINE__, motionDetected, nextMotionEventDue);
        }

        if (UpdateReportedValues()) {
            DEBUGMSG(ZONE_HUBMSG, "%s(%d) - Sending reported values to IoT Hub", FUNC_NAME, __LINE__);
            SendDeviceInfo();
        } else if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
            // Read Sensors ...
            char messagePayload[MESSAGE_MAX_LEN];

            upTime = (int)(SystemTickCounterRead() - deviceStartTime) / 1000;
            
            bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMessageDue || onMeasureNow);

            if (! suppressMessages) {

                // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
                if (strlen(messagePayload) != 0) {
                    char szUpTime[11];
        
                    snprintf(szUpTime, 10, "%d", upTime);
                    EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messagePayload, MESSAGE);
                    DevKitMQTTClient_Event_AddProp(message, JSON_TEMPERATURE_ALERT, temperatureAlert ? "true" : "false");
                    DevKitMQTTClient_Event_AddProp(message, JSON_UPTIME, szUpTime);
                    DevKitMQTTClient_SendEventInstance(message);
                }

                if (nextMessageDue)
                {
                    send_interval_ms = SystemTickCounterRead();     // reset the send interval because we just did send a message
                } 

            }

            measure_interval_ms = SystemTickCounterRead();      // reset regardless of message send after each sensor reading

            if (onMeasureNow) {
                onMeasureNow = false;
            }
            
        } else {
            DEBUGMSG(ZONE_MAINLOOP, "%s(%d) - Checking for Hub Traffic", FUNC_NAME, __LINE__);
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            DEBUGMSG(ZONE_INIT, "%s(%d) - onReset = true", FUNC_NAME, __LINE__)
            onReset = false;
            NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {
            DEBUGMSG(ZONE_INIT, "%s(%d) - onFirmwareUpdate = true", FUNC_NAME, __LINE__)
            onFirmwareUpdate = false;
            CheckNewFirmware();
        }

        DEBUGMSG(ZONE_MAINLOOP, "%s(%d) Sleeping for %d ms", FUNC_NAME, __LINE__, SleepInterval());
        delay(SleepInterval());
    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        DEBUGMSG(ZONE_MAINLOOP, "%s(%d) - Check for IoT Hub activities", FUNC_NAME, __LINE__);
        delay(5000);
        DevKitMQTTClient_Check();
    }
}

