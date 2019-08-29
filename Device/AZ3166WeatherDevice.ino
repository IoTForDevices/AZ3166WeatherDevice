#line 1 "AZ3166WeatherDevice.ino"
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
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
#include "Utils.h"
#include "DebugZones.h"

#include <inttypes.h>

#ifdef LOGGING
DBGPARAM dpCurSettings =
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
static bool onDisplayOn = true;     // Initially show data on the display for 60 seconds.

static bool messageSending = true;
static uint64_t send_interval_ms;
static uint64_t measure_interval_ms;
static uint64_t warming_up_interval_ms;
static uint64_t motion_interval_ms;
static uint64_t deviceStartTime = 0;
static uint64_t upTime = 0;
static uint64_t telemetryMsgs = 0;
static uint64_t sReads = 0;
static uint64_t display_on_interval_ms;

static bool nextMeasurementDue;
static bool firstMessageSend = false;
static bool nextMessageDue;
static bool nextMotionEventDue;
static bool suppressMessages;
static bool bShowDisplay = true;

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
    DEBUGMSG_FUNC_IN("()");

    bool hasWifi = false;

    if (WiFi.begin() == WL_CONNECTED) {
        hasWifi = true;
        Screen.print(1, "Running...");
    } else {
        Screen.print(1, "No Wi-Fi!");
    }
    DEBUGMSG_FUNC_OUT(" = %s", ShowBool(hasWifi));
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
    DEBUGMSG_FUNC_IN("()");
    bool hasIoTHub = DevKitMQTTClient_Init(true);
    if (hasIoTHub) {
        Screen.print(1, "Running....");
    } else {
        Screen.print(1, "No IoTHub!");
    }
    DEBUGMSG_FUNC_OUT(" = %s", ShowBool(hasIoTHub));
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

    DEBUGMSG_FUNC_IN("(%s, %p, %d, %p, %p)", methodName, (void*)payload, length, (void*)*response, (void*)responseLength);

    if (strcmp(methodName, "Reset") == 0) {
        onReset = HandleReset(response, responseLength);
    } else if (strcmp(methodName, "MeasureNow") == 0) {
        onMeasureNow = HandleMeasureNow(response, responseLength);
    } else if (strcmp(methodName, "DisplayOn") == 0) {
        onDisplayOn = HandleDisplayOn(response, responseLength);
        onMeasureNow = true;    // Force the display to be updated immediately.
        display_on_interval_ms = SystemTickCounterRead();
    } else if (strcmp(methodName, "FirmwareUpdate") == 0) {
        onFirmwareUpdate = HandleFirmwareUpdate((const char*)payload, length, response, responseLength);
    } else if (strcmp(methodName, "ResetFWVersion") == 0) {
        onResetFWVersion = HandleResetFWVersion((const char*)payload, length, response, responseLength);
    } else {
        result = 500;
        HandleUnknownMethod(response, responseLength);
    }

    DEBUGMSG_FUNC_OUT(" = %d", result);
    return result;
}

/********************************************************************************************************
 * TwinCallback receives desired Device Twin update settings from IoT Hub.
 * The Desired Values are parsed and if found valid, we will act on those desired values by changing
 * settings, after which we make sure to match the desired value with the reported value from the device.
 *******************************************************************************************************/
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    DEBUGMSG_FUNC_IN("(%d, %p, %d)", (int)updateState, (void*)payLoad, length);
    ParseTwinMessage(updateState, (const char *)payLoad, length);
    DEBUGMSG_FUNC_OUT("");
}

/********************************************************************************************************
 * setup is the default initialisation function for Arduino based devices. It will be executed once
 * after (re)booting the system. During initialization we enable WiFi, connect to the IoT Hub and define
 * the callback functions for direct method execution and desired twin updates.
 *******************************************************************************************************/
void setup()
{
    DEBUGMSG_FUNC_IN("()"); 

    if (!InitWifi()) {
        exit(1);
    }
    if (!InitIoTHub()) {
        exit(1);
    }

    DevKitMQTTClient_SetDeviceMethodCallback(&DeviceMethodCallback);
    DevKitMQTTClient_SetDeviceTwinCallback(&TwinCallback);

    SetupSensors();
    send_interval_ms = measure_interval_ms = warming_up_interval_ms = deviceStartTime = motion_interval_ms = display_on_interval_ms = SystemTickCounterRead();

#ifdef LOGGING
    char szTC[ULONG_64_MAX_DIGITS+1];
    Int64ToString(send_interval_ms, szTC, ULONG_64_MAX_DIGITS);
    DEBUGMSG(ZONE_INIT, "send_interval_ms = measure_interval_ms = warming_up_interval_ms = deviceStartTime = motion_interval_ms = %s", szTC);
#endif
    DEBUGMSG_FUNC_OUT("");
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
        char szTC[ULONG_64_MAX_DIGITS+1];
        char szMI[ULONG_64_MAX_DIGITS+1];
        char szSI[ULONG_64_MAX_DIGITS+1];
        char szMM[ULONG_64_MAX_DIGITS+1];
        char szWU[ULONG_64_MAX_DIGITS+1];
        char szDO[ULONG_64_MAX_DIGITS+1];

        Int64ToString(tickCount, szTC, ULONG_64_MAX_DIGITS);
        Int64ToString(measure_interval_ms, szMI, ULONG_64_MAX_DIGITS);
        Int64ToString(send_interval_ms, szSI, ULONG_64_MAX_DIGITS);
        Int64ToString(motion_interval_ms, szMM, ULONG_64_MAX_DIGITS);
        Int64ToString(warming_up_interval_ms, szWU, ULONG_64_MAX_DIGITS);
        Int64ToString(display_on_interval_ms, szDO, ULONG_64_MAX_DIGITS);
#endif
        int measurementInterval = MeasurementInterval();
        int sendInterval = SendInterval();
        int motionInterval = MotionInterval();
        int sleepInterval = SleepInterval();
        int displayOnInterval = DisplayOnInterval();

        nextMeasurementDue = (tickCount - measure_interval_ms) >= measurementInterval;
        DEBUGMSG(ZONE_MAINLOOP, "nextMeasurementDue = %d, tickCount = %s, measure_interval_ms = %s, measurementInterval = %d", nextMeasurementDue, szTC, szMI, measurementInterval);
        nextMessageDue = (tickCount - send_interval_ms) >= sendInterval;
        DEBUGMSG(ZONE_MAINLOOP, "nextMessageDue = %d, tickCount = %s, send_interval_ms = %s, sendInterval = %d", nextMessageDue, szTC, szSI, sendInterval);
        nextMotionEventDue = (tickCount - motion_interval_ms) >= motionInterval;
        DEBUGMSG(ZONE_MAINLOOP, "nextMotionEventDue = %d, tickCount = %s, motion_interval_ms = %s, motionInterval = %d", nextMotionEventDue, szTC, szMM, motionInterval);

        if (onDisplayOn) {
            bShowDisplay = onDisplayOn = (tickCount - display_on_interval_ms) < displayOnInterval;
            DEBUGMSG(ZONE_MAINLOOP, "bShowDisplay = %s, tickCount = %s, display_on_interval_ms = %s, displayOnInterval = %d", ShowBool(bShowDisplay), szTC, szDO, displayOnInterval);
        }

        if (! firstMessageSend) {
            int warmingUpTime = WarmingUpTime();
            suppressMessages = (tickCount - warming_up_interval_ms) < warmingUpTime;
            DEBUGMSG(ZONE_MAINLOOP, "suppressMessages = %d, SystemTickCounterRead() = %s, warming_up_interval_ms = %s, WarmingUpTime() = %d", suppressMessages, szTC, szWU, warmingUpTime);

            if (! suppressMessages) {
                firstMessageSend = true;
                nextMessageDue = true;
                onMeasureNow = true;
            }
        }

        if (MotionDetectionEnabled()) {
            // Check if the device is in motion or not and enable an alarm if so.
            bool motionDetected = MotionDetected(MotionSensitivity());

            if (motionDetected && nextMotionEventDue) {
                char *pszEventMsg = CreateEventMsg(MOTION_EVENT);
                if (pszEventMsg != NULL) {
                    EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(pszEventMsg, MESSAGE);
                    DevKitMQTTClient_SendEventInstance(message);
                    free(pszEventMsg);
                    motion_interval_ms = SystemTickCounterRead();
                    DEBUGMSG(ZONE_MOTIONDETECT, "Sending Motion Detected Event - motion_inteval_ms = %d", motion_interval_ms);
                }
            }

            DEBUGMSG(ZONE_MOTIONDETECT, "Motion Detected = %d, nextMotionEventDue = %d", motionDetected, nextMotionEventDue);
        }

        if (UpdateReportedValues()) {
            DEBUGMSG(ZONE_HUBMSG, "Sending reported values to IoT Hub");
            SendDeviceInfo();
        } else if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
            // Read Sensors ...
            char *pszMsgPayLoad = NULL;
            char szUp[ULONG_64_MAX_DIGITS+1];

            upTime = (uint64_t)((SystemTickCounterRead() - deviceStartTime) / 1000);

            bool temperatureAlert = CreateTelemetryMessage(&pszMsgPayLoad, upTime, nextMessageDue || onMeasureNow, bShowDisplay);
   
            sReads = sReads + 1;

            if (! suppressMessages) {

                // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
                if (pszMsgPayLoad != NULL) {
                    char szDeviceName[21];
        
                    snprintf(szDeviceName, sizeof(szDeviceName) - 1, "%s", CurrentDevice());
                    EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(pszMsgPayLoad, MESSAGE);
                    DevKitMQTTClient_Event_AddProp(message, JSON_TEMPERATURE_ALERT, temperatureAlert ? "true" : "false");
                    DevKitMQTTClient_Event_AddProp(message, JSON_DEVICEID, szDeviceName);
                    free(pszMsgPayLoad);
                    DEBUGMSG(ZONE_HUBMSG, "Sending telemetry values to IoT Hub for %s", szDeviceName);

                    DevKitMQTTClient_SendEventInstance(message);
                    telemetryMsgs = telemetryMsgs + 1;
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
            DEBUGMSG(ZONE_MAINLOOP, "Checking for Hub Traffic");
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            DEBUGMSG(ZONE_INIT, "onReset = true");
            onReset = false;
            NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {
            DEBUGMSG(ZONE_INIT, "onFirmwareUpdate = true");
            onFirmwareUpdate = false;
            CheckNewFirmware();
        }

        if (onResetFWVersion) {
            DEBUGMSG(ZONE_INIT, "onResetFWVersion = true");
            onResetFWVersion = false;
            CheckResetFirmwareInfo();
        }

        DEBUGMSG(ZONE_MAINLOOP, "Sleeping for %d ms", sleepInterval);
        delay(sleepInterval);
    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        DEBUGMSG(ZONE_MAINLOOP, "Check for IoT Hub activities");
        delay(5000);
        DevKitMQTTClient_Check();
    }
}
