#line 1 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
#line 1 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
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

static bool onReset = false;

static bool messageSending = true;
static uint64_t send_interval_ms;
static uint64_t measure_interval_ms;
static uint64_t warming_up_interval_ms;
static bool reportProperties = false;

static DEVICE_SETTINGS deviceSettings { DEFAULT_MEASURE_INTERVAL, DEFAULT_SEND_INTERVAL, DEFAULT_WARMING_UP_TIME, 
                                        DEFAULT_MEASURE_INTERVAL_MSEC, DEFAULT_SEND_INTERVAL_MSEC, DEFAULT_WARMING_UP_TIME_MSEC,
                                        DEFAULT_TEMPERATURE_ALERT,
                                        DEFAULT_TEMPERATURE_ACCURACY, DEFAULT_PRESSURE_ACCURACY, DEFAULT_HUMIDITY_ACCURACY,
                                        DEFAULT_MAX_DELTA_BETWEEN_MEASUREMENTS,
                                        0.0, 0.0, 0.0 };

#line 33 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length);
#line 49 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
bool InitWifi();
#line 62 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
bool InitIoTHub();
#line 76 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength);
#line 90 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
void setup();
#line 108 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
void loop();
#line 33 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length){
    char payLoadString[length+1];
    snprintf(payLoadString, length, "%s", payLoad);
    LogInfo("    Payload: Length = %d, Content = %s", length, payLoadString);

    char *temp = (char *)malloc(length + 1);
    if (temp == NULL)
    {
        return;
    }
    memcpy(temp, payLoad, length);
    temp[length] = '\0';
    reportProperties = ParseTwinMessage(updateState, temp, &deviceSettings);
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

    LogInfo("DeviceMethodCallback!");
    if (strcmp(methodName,"Reset") == 0) {
        onReset = HandleReset(response, responseLength);
    } else {
        result = 500;
        HandleUnknownMethod(response, responseLength);
    }
    return result;
}

void setup()
{
    if (!InitWifi()) {
        exit(1);
    }
    if (!InitIoTHub()) {
        exit(1);
    }

    DevKitMQTTClient_SetDeviceMethodCallback(&DeviceMethodCallback);
    DevKitMQTTClient_SetDeviceTwinCallback(&TwinCallback);

    SendDeviceInfo(&deviceSettings);

    SetupSensors();
    send_interval_ms = measure_interval_ms = warming_up_interval_ms = SystemTickCounterRead();
}

void loop()
{
    bool nextMeasurementDue = (int)(SystemTickCounterRead() - measure_interval_ms) >= deviceSettings.mImsec;
    bool nextMessageDue = (int)(SystemTickCounterRead() - send_interval_ms) >= deviceSettings.sImsec;
    bool suppressMessages = false;

    if (deviceSettings.warmingUpTime != 0) {
        suppressMessages = (int)(SystemTickCounterRead() - warming_up_interval_ms) < deviceSettings.wUTmsec;

        if (! suppressMessages) {
            deviceSettings.wUTmsec = 0;
            deviceSettings.warmingUpTime = 0;
            nextMessageDue = true;
        }
    }

    if (nextMeasurementDue || nextMessageDue) {
        // Read Sensors ...
        char messagePayload[MESSAGE_MAX_LEN];

        bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMessageDue, &deviceSettings);

        if (! suppressMessages) {

            // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
            if (strlen(messagePayload) != 0) {
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messagePayload, MESSAGE);
                DevKitMQTTClient_Event_AddProp(message, "temperatureAlert", temperatureAlert ? "true" : "false");
                DevKitMQTTClient_SendEventInstance(message);
                send_interval_ms = SystemTickCounterRead();     // reset the send interval because we just did send a message 
            }
        }

        measure_interval_ms = SystemTickCounterRead();      // reset regardless of message send after each sensor reading
    } else if (reportProperties) {
        SendDeviceInfo(&deviceSettings);
        reportProperties = false;
    } else {
        DevKitMQTTClient_Check();
    }

    if (IsButtonClicked(USER_BUTTON_A)) {
        char messageEvt[MESSAGE_MAX_LEN];
        CreateEventMsg(messageEvt, WARNING_EVENT);

        EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
        DevKitMQTTClient_SendEventInstance(message);
    }

    if (IsButtonClicked(USER_BUTTON_B)) {
        char messageEvt[MESSAGE_MAX_LEN];
        CreateEventMsg(messageEvt, ERROR_EVENT);

        EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
        DevKitMQTTClient_SendEventInstance(message);
    }

    if (onReset) {
        onReset = false;
        NVIC_SystemReset();
    }

    delay(DEFAULT_WAKEUP_INTERVAL);
}
