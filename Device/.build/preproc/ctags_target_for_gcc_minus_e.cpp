# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 2 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 3 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 4 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2

# 6 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2




static RGB_LED rgbLed;
static char displayBuffer[128];

void ShowTelemetryData(float temperature, float humidity, float pressure, bool showHumidity, bool showPressure)
{
    snprintf(displayBuffer, 128, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
        f2s(temperature, 1), showHumidity ? f2s(humidity,1) : "-", showPressure ? f2s(pressure,1) : "-");
    Screen.print(displayBuffer);
}

void BlinkLED()
{
    rgbLed.turnOff();
    rgbLed.setColor(32, 0, 0);
    delay(500);
    rgbLed.turnOff();
}

void BlinkSendConfirmation()
{
    rgbLed.turnOff();
    rgbLed.setColor(0, 0, 32);
    delay(500);
    rgbLed.turnOff();
}
# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
// To get started please visit https://microsoft.github.io/azure-iot-developer-kit/docs/projects/remote-monitoring/?utm_source=ArduinoExtension&utm_medium=ReleaseNote&utm_campaign=VSCode
# 5 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 6 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 7 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 8 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 9 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 11 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 12 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 13 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 15 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 16 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 18 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 19 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 20 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

extern "C" DBGPARAM dpCurSettings =
{
    "AZ3166WeatherDevice",
    {
        "Init", "Function", "Memory", "Twin Parsing",
        "IoT Hub Msg Handling", "Raw Data", "Sensor Data", "Main Loop",
        "Device Methods", "Motion Detection", "Firmware OTA Update", "",
        "Verbose", "Info", "Warning", "Error"
    },
    (0x00000001<<(3)) | (0x00000001<<(7)) | (0x00000001<<(0)) | (0x00000001<<(15)) | (0x00000001<<(5)) | (0x00000001<<(8)) | (0x00000001<<(10))
};



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
    { if ((dpCurSettings.ulZoneMask & (0x00000001<<(0)))) { do{{ if (0) { (void)printf("TwinCallback - Payload: Length = %d", length); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 64 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
   __null
# 64 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 64, 0x01, "TwinCallback - Payload: Length = %d", length); } }; }while((void)0,0); delay(200); } }
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
# 95 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength)
{
    int result = 200;

    { if ((dpCurSettings.ulZoneMask & (0x00000001<<(0)))) { do{{ if (0) { (void)printf("DeviceMethodCallback - methodName = %s", methodName); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 99 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
   __null
# 99 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 99, 0x01, "DeviceMethodCallback - methodName = %s", methodName); } }; }while((void)0,0); delay(200); } };

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

        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("loop nextMeasurementDue = %d, nextMessageDue = %d, nextMotionEventDue = %d, suppressMessages = %d", nextMeasurementDue, nextMessageDue, nextMotionEventDue, suppressMessages); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 167 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 167 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 167, 0x01, "loop nextMeasurementDue = %d, nextMessageDue = %d, nextMotionEventDue = %d, suppressMessages = %d", nextMeasurementDue, nextMessageDue, nextMotionEventDue, suppressMessages); } }; }while((void)0,0); delay(200); } };

        if (MotionDetectionEnabled()) {
            // Check if the device is in motion or not and enable an alarm if so.
            bool motionDetected = MotionDetected(MotionSensitivity());

            if (motionDetected && nextMotionEventDue) {
                char messageEvt[256];
                CreateEventMsg(messageEvt, MOTION_EVENT);
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
                DevKitMQTTClient_SendEventInstance(message);

                motion_interval_ms = SystemTickCounterRead();
                { if ((dpCurSettings.ulZoneMask & (0x00000001<<(9)))) { do{{ if (0) { (void)printf("Sending Motion Detected Event - motion_inteval_ms = %d", motion_interval_ms); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 180 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
               __null
# 180 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
               ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 180, 0x01, "Sending Motion Detected Event - motion_inteval_ms = %d", motion_interval_ms); } }; }while((void)0,0); delay(200); } };
            }

            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(9)))) { do{{ if (0) { (void)printf("Motion Detected = %d, nextMotionEventDue = %d", motionDetected, nextMotionEventDue); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 183 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 183 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 183, 0x01, "Motion Detected = %d, nextMotionEventDue = %d", motionDetected, nextMotionEventDue); } }; }while((void)0,0); delay(200); } };
        }

        if (UpdateReportedValues()) {
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s", "Sending reported values to IoT Hub"); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 187 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 187 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 187, 0x01, "%s", "Sending reported values to IoT Hub"); } }; }while((void)0,0); delay(200); } };
            SendDeviceInfo();
        } else if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
            // Read Sensors ...
            char messagePayload[256];

            upTime = (int)(SystemTickCounterRead() - deviceStartTime) / 1000;

            bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMeasurementDue || onMeasureNow);

            if (! suppressMessages) {

                // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
                if (strlen(messagePayload) != 0) {
                    char szUpTime[11];

                    snprintf(szUpTime, 10, "%d", upTime);
                    EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messagePayload, MESSAGE);
                    DevKitMQTTClient_Event_AddProp(message, "temperatureAlert", temperatureAlert ? "true" : "false");
                    DevKitMQTTClient_Event_AddProp(message, "upTime", szUpTime);
                    DevKitMQTTClient_SendEventInstance(message);

                    if (nextMessageDue)
                    {
                        send_interval_ms = SystemTickCounterRead(); // reset the send interval because we just did send a message
                    }
                }
            }

            measure_interval_ms = SystemTickCounterRead(); // reset regardless of message send after each sensor reading

            if (onMeasureNow) {
                onMeasureNow = false;
            }

        } else {
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s", "Checking for Hub Traffic"); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 223 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 223 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 223, 0x01, "%s", "Checking for Hub Traffic"); } }; }while((void)0,0); delay(200); } };
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            onReset = false;
            __NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(0)))) { do{{ if (0) { (void)printf("onFirmwareUpdate - %s", "Ready to call CheckNewFirmware"); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 233 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 233 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 233, 0x01, "onFirmwareUpdate - %s", "Ready to call CheckNewFirmware"); } }; }while((void)0,0); delay(200); } }
            onFirmwareUpdate = false;
            CheckNewFirmware();
        }





        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("Sleeping for %d ms", SleepInterval()); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 242 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 242 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 242, 0x01, "Sleeping for %d ms", SleepInterval()); } }; }while((void)0,0); delay(200); } };
        delay(SleepInterval());

    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s", "No desired twin values, so wait 5000 ms and only check for IoT Hub activities"); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 247 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 247 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 247, 0x01, "%s", "No desired twin values, so wait 5000 ms and only check for IoT Hub activities"); } }; }while((void)0,0); delay(200); } };
        delay(5000);
        DevKitMQTTClient_Check();
    }
}
