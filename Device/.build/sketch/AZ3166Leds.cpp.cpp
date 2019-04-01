#line 1 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
#line 1 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
#include "Arduino.h"
#include "RGB_LED.h"
#include "AzureIotHub.h"

#include "IoTHubMessageHandling.h"

#define RGB_LED_BRIGHTNESS 32
#define DISPLAY_SIZE 128

static RGB_LED rgbLed;
static char displayBuffer[DISPLAY_SIZE];

#line 13 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void ShowTelemetryData(float temperature, float humidity, float pressure, DEVICE_SETTINGS *pDeviceSettings);
#line 20 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void BlinkLED();
#line 28 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void BlinkSendConfirmation();
#line 50 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length);
#line 70 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
bool InitWifi();
#line 83 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
bool InitIoTHub();
#line 97 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength);
#line 118 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void setup();
#line 155 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void loop();
#line 13 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void ShowTelemetryData(float temperature, float humidity, float pressure, DEVICE_SETTINGS *pDeviceSettings)
{
    snprintf(displayBuffer, DISPLAY_SIZE, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
        f2s(temperature, 1), pDeviceSettings->enableHumidityReading ? f2s(humidity,1) : "-", pDeviceSettings->enablePressureReading ? f2s(pressure,1) : "-");
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

#line 1 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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

#define DIAGNOSTIC_INFO_MAINMODULE_NOT
#define DIAGNOSTIC_INFO_MAINMODULE_LOOP_NOT
#define DIAGNOSTIC_INFO_MAINMODULE_MOTION_NOT

static bool onReset = false;
static bool onMeasureNow = false;
static bool onFirmwareUpdate = false;

static bool messageSending = true;
static uint64_t send_interval_ms;
static uint64_t measure_interval_ms;
static uint64_t warming_up_interval_ms;
static uint64_t motion_interval_ms;
static uint64_t deviceStartTime = 0;

static bool nextMeasurementDue;
static bool nextMessageDue;
static bool nextMotionEventDue;
static bool suppressMessages;
static bool hasSendInitialReportedDeviceTwinValues = false;

static DEVICE_SETTINGS reportedDeviceSettings { DEFAULT_MEASURE_INTERVAL, DEFAULT_SEND_INTERVAL, DEFAULT_SLEEP_INTERVAL, DEFAULT_WARMING_UP_TIME, 0,
                                                DEFAULT_MEASURE_INTERVAL_MSEC, DEFAULT_SEND_INTERVAL_MSEC, DEFAULT_WARMING_UP_TIME_MSEC, DEFAULT_WAKEUP_INTERVAL,
                                                DEFAULT_TEMPERATURE_ALERT,
                                                DEFAULT_TEMPERATURE_ACCURACY, DEFAULT_PRESSURE_ACCURACY, DEFAULT_HUMIDITY_ACCURACY,
                                                DEFAULT_MAX_DELTA_BETWEEN_MEASUREMENTS,
                                                0.0, 0.0, 0.0, DEFAULT_MOTION_SENSITIVITY };
static DEVICE_PROPERTIES reportedDeviceProperties { NULL, NULL, NULL };                                                
static DEVICE_SETTINGS desiredDeviceSettings;

void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    char payLoadString[length+1];
    snprintf(payLoadString, length, "%s", payLoad);

#ifdef DIAGNOSTIC_INFO_MAINMODULE
    LogInfo("    TwinCallback - Payload: Length = %d", length);
    delay(200);
#endif
    char *temp = (char *)malloc(length + 1);
    if (temp == NULL)
    {
        return;
    }
    memcpy(temp, payLoad, length);
    temp[length] = '\0';
    ParseTwinMessage(updateState, temp, &desiredDeviceSettings, &reportedDeviceSettings, &reportedDeviceProperties);
    free(temp);
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

#ifdef DIAGNOSTIC_INFO_MAINMODULE
    LogInfo("DeviceMethodCallback!");
#endif

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
    int nLength = strlen(DEVICE_ID);
    reportedDeviceProperties.pszDeviceModel = (char *)malloc(nLength + 1);
    if (reportedDeviceProperties.pszDeviceModel == NULL) {
        exit(1);
    }
    snprintf(reportedDeviceProperties.pszDeviceModel, nLength + 1, "%s", DEVICE_ID);

    nLength = strlen(DEVICE_LOCATION);
    reportedDeviceProperties.pszLocation = (char *)malloc(nLength + 1);
    if (reportedDeviceProperties.pszLocation == NULL) {
        exit(1);
    }
    snprintf(reportedDeviceProperties.pszLocation, nLength + 1, "%s", DEVICE_LOCATION);

    nLength = strlen(DEVICE_FIRMWARE_VERSION);
    reportedDeviceProperties.pszCurrentFwVersion = (char *)malloc(nLength + 1);
    if (reportedDeviceProperties.pszCurrentFwVersion == NULL) {
        exit(1);
    }
    snprintf(reportedDeviceProperties.pszCurrentFwVersion, nLength + 1, "%s", DEVICE_FIRMWARE_VERSION);

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
        nextMeasurementDue = (SystemTickCounterRead() - measure_interval_ms) >= reportedDeviceSettings.mImsec;
        nextMessageDue = (SystemTickCounterRead() - send_interval_ms) >= reportedDeviceSettings.sImsec;
        nextMotionEventDue = (SystemTickCounterRead() - motion_interval_ms) >= reportedDeviceSettings.motionInMsec;

        if (reportedDeviceSettings.warmingUpTime != 0) {
            suppressMessages = (SystemTickCounterRead() - warming_up_interval_ms) < reportedDeviceSettings.wUTmsec;

            if (! suppressMessages) {
                reportedDeviceSettings.wUTmsec = 0;
                reportedDeviceSettings.warmingUpTime = 0;
                nextMessageDue = true;
            }
        }

#ifdef DIAGNOSTIC_INFO_MAINMODULE_LOOP
        LogInfo("loop nextMeasurementDue = %d, nextMessageDue = %d, nextMotionEventDue = %d, suppressMessages = %d", nextMeasurementDue, nextMessageDue, nextMotionEventDue, suppressMessages);
        delay(200);
#endif

        if (reportedDeviceSettings.enableMotionDetection) {
            // Check if the device is in motion or not and enable an alarm if so.
            bool motionDetected = MotionDetected(reportedDeviceSettings.motionSensitivity);

            if (motionDetected && nextMotionEventDue) {
                char messageEvt[MESSAGE_MAX_LEN];
                CreateEventMsg(messageEvt, MOTION_EVENT);
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
                DevKitMQTTClient_SendEventInstance(message);

                motion_interval_ms = SystemTickCounterRead();
#ifdef DIAGNOSTIC_INFO_MAINMODULE_MOTION
                LogInfo("Sending Motion Detected Event - motion_inteval_ms = %d", motion_interval_ms);
#endif        

            }
#ifdef DIAGNOSTIC_INFO_MAINMODULE_MOTION
            LogInfo("Motion Detected = %d, nextMotionEventDue = %d", motionDetected, nextMotionEventDue);
#endif        
        }

        if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
            // Read Sensors ...
            char messagePayload[MESSAGE_MAX_LEN];

            reportedDeviceSettings.upTime = (int)(SystemTickCounterRead() - deviceStartTime) / 1000;
            
            bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMessageDue || onMeasureNow, &reportedDeviceSettings);

            if (! suppressMessages) {

                // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
                if (strlen(messagePayload) != 0) {
                    char szUpTime[11];
        
                    snprintf(szUpTime, 10, "%d", reportedDeviceSettings.upTime);
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
            
        } else if (SendReportedDeviceTwinValues() && ! hasSendInitialReportedDeviceTwinValues) {
            SendDeviceInfo(&reportedDeviceSettings, &reportedDeviceProperties);
            hasSendInitialReportedDeviceTwinValues = true;
        } else {
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            onReset = false;
            NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {
#ifdef DIAGNOSTIC_INFO_MAINMODULE        
            LogInfo("Ready to call CheckNewFirmware");
#endif
            onFirmwareUpdate = false;
            CheckNewFirmware();
        }

#ifdef DIAGNOSTIC_INFO_MAINMODULE
        delay(2000);
#else
        delay(reportedDeviceSettings.dSmsec);
#endif
    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        delay(5000);
        DevKitMQTTClient_Check();
    }
}

