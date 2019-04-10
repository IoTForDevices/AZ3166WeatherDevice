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
#line 62 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length);
#line 68 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
bool InitWifi();
#line 81 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
bool InitIoTHub();
#line 95 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength);
#line 114 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void setup();
#line 151 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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

#define DIAGNOSTIC_INFO_MAINMODULE_NOT

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

// static DEVICE_SETTINGS reportedDeviceSettings { DEFAULT_MEASURE_INTERVAL, DEFAULT_SEND_INTERVAL, DEFAULT_SLEEP_INTERVAL, DEFAULT_WARMING_UP_TIME, 0,
//                                                 DEFAULT_MEASURE_INTERVAL_MSEC, DEFAULT_SEND_INTERVAL_MSEC, DEFAULT_WARMING_UP_TIME_MSEC, DEFAULT_WAKEUP_INTERVAL,
//                                                 DEFAULT_TEMPERATURE_ALERT,
//                                                 DEFAULT_TEMPERATURE_ACCURACY, DEFAULT_PRESSURE_ACCURACY, DEFAULT_HUMIDITY_ACCURACY,
//                                                 DEFAULT_MAX_DELTA_BETWEEN_MEASUREMENTS,
//                                                 0.0, 0.0, 0.0, DEFAULT_MOTION_SENSITIVITY };
// static DEVICE_PROPERTIES reportedDeviceProperties { NULL, NULL, NULL };                                                
// static DEVICE_SETTINGS desiredDeviceSettings;

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

    DEBUGMSG(ZONE_INIT, "DeviceMethodCallback - methodName = %s", methodName);

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

void loop()
{
    if (InitialDeviceTwinDesiredReceived()) {
        nextMeasurementDue = (SystemTickCounterRead() - measure_interval_ms) >= MeasurementInterval();
        nextMessageDue = (SystemTickCounterRead() - send_interval_ms) >= SleepInterval();
        nextMotionEventDue = (SystemTickCounterRead() - motion_interval_ms) >= MotionInterval();

        if (! firstMessageSend) {
            suppressMessages = (SystemTickCounterRead() - warming_up_interval_ms) < WarmingUpTime();

            if (! suppressMessages) {
                firstMessageSend = true;
                nextMeasurementDue = true;
            }
        }

        DEBUGMSG(ZONE_MAINLOOP, "loop nextMeasurementDue = %d, nextMessageDue = %d, nextMotionEventDue = %d, suppressMessages = %d", nextMeasurementDue, nextMessageDue, nextMotionEventDue, suppressMessages);

        if (MotionDetectionEnabled()) {
            // Check if the device is in motion or not and enable an alarm if so.
            bool motionDetected = MotionDetected(MotionSensitivity());

            if (motionDetected && nextMotionEventDue) {
                char messageEvt[MESSAGE_MAX_LEN];
                CreateEventMsg(messageEvt, MOTION_EVENT);
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
                DevKitMQTTClient_SendEventInstance(message);

                motion_interval_ms = SystemTickCounterRead();
                DEBUGMSG(ZONE_MOTIONDETECT, "Sending Motion Detected Event - motion_inteval_ms = %d", motion_interval_ms);
            }

            DEBUGMSG(ZONE_MOTIONDETECT, "Motion Detected = %d, nextMotionEventDue = %d", motionDetected, nextMotionEventDue);
        }

        if (UpdateReportedValues()) {
            DEBUGMSG(ZONE_MAINLOOP, "%s", "Sending reported values to IoT Hub");
            SendDeviceInfo();
        } else if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
            // Read Sensors ...
            char messagePayload[MESSAGE_MAX_LEN];

            upTime = (int)(SystemTickCounterRead() - deviceStartTime) / 1000;
            
            bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMeasurementDue || onMeasureNow);

            if (! suppressMessages) {

                // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
                if (strlen(messagePayload) != 0) {
                    char szUpTime[11];
        
                    snprintf(szUpTime, 10, "%d", upTime);
                    EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messagePayload, MESSAGE);
                    DevKitMQTTClient_Event_AddProp(message, JSON_TEMPERATURE_ALERT, temperatureAlert ? "true" : "false");
                    DevKitMQTTClient_Event_AddProp(message, JSON_UPTIME, szUpTime);
                    DevKitMQTTClient_SendEventInstance(message);

                    if (nextMessageDue)
                    {
                        send_interval_ms = SystemTickCounterRead();     // reset the send interval because we just did send a message
                    } 
                }
            }

            measure_interval_ms = SystemTickCounterRead();      // reset regardless of message send after each sensor reading

            if (onMeasureNow) {
                onMeasureNow = false;
            }
            
        } else {
            DEBUGMSG(ZONE_MAINLOOP, "%s", "Checking for Hub Traffic");
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            onReset = false;
            NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {
            DEBUGMSG(ZONE_INIT, "onFirmwareUpdate - %s", "Ready to call CheckNewFirmware")
            onFirmwareUpdate = false;
            CheckNewFirmware();
        }

#ifdef DIAGNOSTIC_INFO_MAINMODULE
        DEBUGMSG(ZONE_MAINLOOP, "%s", "Sleeping for 2000 ms");
        delay(2000);
#else
        DEBUGMSG(ZONE_MAINLOOP, "Sleeping for %d ms", SleepInterval());
        delay(SleepInterval());
#endif
    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        DEBUGMSG(ZONE_MAINLOOP, "%s", "No desired twin values, so wait 5000 ms and only check for IoT Hub activities");
        delay(5000);
        DevKitMQTTClient_Check();
    }
}

