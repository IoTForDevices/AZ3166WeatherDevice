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
static bool onResetFWVersion = false;

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

/**********************************************************************************************************
 * InitWifi
 * 
 * This method is used to check if we are connected to a WiFi network. If so, continue Running
 * normal operations, including starting measuring temperature, humidity, pressure. If we are not
 * connected to a WiFi network, just indicate this on the screen and be non-operational.
 * 
 *********************************************************************************************************/
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

/**********************************************************************************************************
 * InitIoTHub
 * 
 * This method is used to initially connect to an IoT Hub. If connected, continue Running
 * normal operations, including starting measuring temperature, humidity, pressure. If we are not
 * connected to an IoT Hub, just indicate this on the screen and be non-operational.
 * 
 *********************************************************************************************************/
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

/********************************************************************************************************
 * IoT Hub Callback functions.
 * NOTE: These functions must be available inside this source file, prior to the Init and Loop methods.
 ********************************************************************************************************/

/********************************************************************************************************
 * DeviceMethodCallback receives direct commands from IoT Hub to be executed on the device
 * Methods can optionally contain parameters (passed in the payload parameter) and will send response
 * messages to IoT Hub as well.
 * 
 * The way this has been implemented is that we call a function with the name HandleXXX. These functions
 * can be found in the source file IoTHubDeviceMethods.cpp. All these functions do the necessary
 * parameter retrieval and they also build a response string. The actual requested functionality that
 * needs to be executed as a result of a direct command is handled by the main program loop. If all
 * paratmers are retrieved sucessfully, a boolean flag is set that will execute code in the main loop,
 * after which the boolean flag will be reset again.
 *******************************************************************************************************/
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
    } else if (strcmp(methodName, "ResetFWVersion") == 0) {
        onResetFWVersion = HandleResetFWVersion((const char*)payload, length, response, responseLength);
    } else {
        result = 500;
        HandleUnknownMethod(response, responseLength);
    }
    return result;
}

/********************************************************************************************************
 * TwinCallback receives desired Device Twin update settings from IoT Hub.
 * The Desired Values are parsed and if found valid, we will act on those desired values by changing
 * settings, after which we make sure to match the desired value with the reported value from the device.
 *******************************************************************************************************/
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    DEBUGMSG(ZONE_INIT, "TwinCallback - Payload: Length = %d", length)
    ParseTwinMessage(updateState, (const char *)payLoad, length);
}

/********************************************************************************************************
 * Int64ToString is a helper function that allows easy display / logging of long integers. These long integers
 * are mainly used in this application to find out when timers are expired in the main loop, after which
 * some action needs to be taken.
 * 
 * NOTE: This function is only available when LOGGING is enabled.
 *******************************************************************************************************/
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

/********************************************************************************************************
 * setup is the default initialisation function for Arduino based devices. It will be executed once
 * after (re)booting the system. During initialization we enable WiFi, connect to the IoT Hub and define
 * the callback functions for direct method execution and desired twin updates.
 *******************************************************************************************************/
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

    SetupSensors();
    send_interval_ms = measure_interval_ms = warming_up_interval_ms = deviceStartTime = motion_interval_ms = SystemTickCounterRead();
}

/********************************************************************************************************
 * loop is the default processing loop function for Arduino based devices. It will be executed every time
 * a sleep interval expires. When executing, it checks if there needs to be telemetry data send to the
 * IoT Hub, if code needs to be executed due to direct method calls and polls to find out if IoT Hub
 * requests something from us.
 *******************************************************************************************************/
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
                // Send reported property to indicate that we just read all sensor values.
                
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

        if (onResetFWVersion) {
            DEBUGMSG(ZONE_INIT, "%s(%d) - onResetFWVersion = true", FUNC_NAME, __LINE__)
            onResetFWVersion = false;
            CheckResetFirmwareInfo();
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
