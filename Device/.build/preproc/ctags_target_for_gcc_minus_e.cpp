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

# 22 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2



# 24 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
extern "C" DBGPARAM dpCurSettings =
{
    "AZ3166WeatherDevice",
    {
        "Init", "Function", "Memory", "Twin Parsing",
        "IoT Hub Msg Handling", "Raw Data", "Sensor Data", "Main Loop",
        "Device Methods", "Motion Detection", "Firmware OTA Update", "",
        "Verbose", "Info", "Warning", "Error"
    },
    (0x00000001<<(3)) | (0x00000001<<(7)) | (0x00000001<<(15)) | (0x00000001<<(14))
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

void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    { if ((dpCurSettings.ulZoneMask & (0x00000001<<(0)))) { do{{ if (0) { (void)printf("TwinCallback - Payload: Length = %d", length); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 57 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
   __null
# 57 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 57, 0x01, "TwinCallback - Payload: Length = %d", length); } }; }while((void)0,0); delay(200); } }
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
# 88 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength)
{
    int result = 200;

    { if ((dpCurSettings.ulZoneMask & (0x00000001<<(0)))) { do{{ if (0) { (void)printf("--> %s(methodName = %s)",__func__, methodName); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 92 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
   __null
# 92 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 92, 0x01, "--> %s(methodName = %s)",__func__, methodName); } }; }while((void)0,0); delay(200); } };

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


void Int64ToString(uint64_t uiValue, char *pszValue)
{
    const int NUM_DIGITS = log10(uiValue) + 1;
    *(pszValue+NUM_DIGITS) = '\0';

    for (size_t i = NUM_DIGITS; i--; uiValue /= 10) {
        *(pszValue+i) = '0' + (uiValue % 10);
    }
}


void loop()
{
    if (InitialDeviceTwinDesiredReceived()) {
        uint64_t tickCount = SystemTickCounterRead();

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

        nextMeasurementDue = (tickCount - measure_interval_ms) >= MeasurementInterval();
        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s(%d), nextMeasurementDue = %d, tickCount = %s, measure_interval_ms = %s, MeasurementInterval() = %d",__func__, 174, nextMeasurementDue, szTC, szMI, MeasurementInterval()); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 174 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 174 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 174, 0x01, "%s(%d), nextMeasurementDue = %d, tickCount = %s, measure_interval_ms = %s, MeasurementInterval() = %d",__func__, 174, nextMeasurementDue, szTC, szMI, MeasurementInterval()); } }; }while((void)0,0); delay(200); } };
        nextMessageDue = (tickCount - send_interval_ms) >= SendInterval();
        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s(%d), nextMessageDue = %d, tickCount = %s, send_interval_ms = %s, SendInterval() = %d",__func__, 176, nextMessageDue, szTC, szSI, SendInterval()); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 176 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 176 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 176, 0x01, "%s(%d), nextMessageDue = %d, tickCount = %s, send_interval_ms = %s, SendInterval() = %d",__func__, 176, nextMessageDue, szTC, szSI, SendInterval()); } }; }while((void)0,0); delay(200); } };
        nextMotionEventDue = (tickCount - motion_interval_ms) >= MotionInterval();
        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s(%d), nextMotionEventDue = %d, tickCount = %s, motion_interval_ms = %s, MotionInterval() = %d",__func__, 178, nextMotionEventDue, szTC, szMM, MotionInterval()); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 178 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 178 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 178, 0x01, "%s(%d), nextMotionEventDue = %d, tickCount = %s, motion_interval_ms = %s, MotionInterval() = %d",__func__, 178, nextMotionEventDue, szTC, szMM, MotionInterval()); } }; }while((void)0,0); delay(200); } };

        if (! firstMessageSend) {
            suppressMessages = (tickCount - warming_up_interval_ms) < WarmingUpTime();
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s(%d), suppressMessages = %d, SystemTickCounterRead() = %s, warming_up_interval_ms = %s, WarmingUpTime() = %d",__func__, 182, suppressMessages, szTC, szWU, WarmingUpTime()); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 182 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 182 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 182, 0x01, "%s(%d), suppressMessages = %d, SystemTickCounterRead() = %s, warming_up_interval_ms = %s, WarmingUpTime() = %d",__func__, 182, suppressMessages, szTC, szWU, WarmingUpTime()); } }; }while((void)0,0); delay(200); } };

            if (! suppressMessages) {
                firstMessageSend = true;
                nextMessageDue = true;
            }
        }

        if (MotionDetectionEnabled()) {
            // Check if the device is in motion or not and enable an alarm if so.
            bool motionDetected = MotionDetected(MotionSensitivity());

            if (motionDetected && nextMotionEventDue) {
                char messageEvt[256];
                CreateEventMsg(messageEvt, MOTION_EVENT);
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
                DevKitMQTTClient_SendEventInstance(message);

                motion_interval_ms = SystemTickCounterRead();
                { if ((dpCurSettings.ulZoneMask & (0x00000001<<(9)))) { do{{ if (0) { (void)printf("%s(%d) - Sending Motion Detected Event - motion_inteval_ms = %d",__func__, 201, motion_interval_ms); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 201 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
               __null
# 201 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
               ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 201, 0x01, "%s(%d) - Sending Motion Detected Event - motion_inteval_ms = %d",__func__, 201, motion_interval_ms); } }; }while((void)0,0); delay(200); } };
            }

            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(9)))) { do{{ if (0) { (void)printf("%s(%d) - Motion Detected = %d, nextMotionEventDue = %d",__func__, 204, motionDetected, nextMotionEventDue); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 204 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 204 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 204, 0x01, "%s(%d) - Motion Detected = %d, nextMotionEventDue = %d",__func__, 204, motionDetected, nextMotionEventDue); } }; }while((void)0,0); delay(200); } };
        }

        if (UpdateReportedValues()) {
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(4)))) { do{{ if (0) { (void)printf("%s(%d) - Sending reported values to IoT Hub",__func__, 208); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 208 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 208 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 208, 0x01, "%s(%d) - Sending reported values to IoT Hub",__func__, 208); } }; }while((void)0,0); delay(200); } };
            SendDeviceInfo();
        } else if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
            // Read Sensors ...
            char messagePayload[256];

            upTime = (int)(SystemTickCounterRead() - deviceStartTime) / 1000;

            bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMessageDue || onMeasureNow);

            if (! suppressMessages) {

                // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
                if (strlen(messagePayload) != 0) {
                    char szUpTime[11];

                    snprintf(szUpTime, 10, "%d", upTime);
                    EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messagePayload, MESSAGE);
                    DevKitMQTTClient_Event_AddProp(message, "temperatureAlert", temperatureAlert ? "true" : "false");
                    DevKitMQTTClient_Event_AddProp(message, "upTime", szUpTime);
                    DevKitMQTTClient_SendEventInstance(message);
                }

                if (nextMessageDue)
                {
                    send_interval_ms = SystemTickCounterRead(); // reset the send interval because we just did send a message
                }

            }

            measure_interval_ms = SystemTickCounterRead(); // reset regardless of message send after each sensor reading

            if (onMeasureNow) {
                onMeasureNow = false;
            }

        } else {
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s(%d) - Checking for Hub Traffic",__func__, 245); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 245 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 245 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 245, 0x01, "%s(%d) - Checking for Hub Traffic",__func__, 245); } }; }while((void)0,0); delay(200); } };
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(0)))) { do{{ if (0) { (void)printf("%s(%d) - onReset = true",__func__, 250); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 250 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 250 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 250, 0x01, "%s(%d) - onReset = true",__func__, 250); } }; }while((void)0,0); delay(200); } }
            onReset = false;
            __NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {
            { if ((dpCurSettings.ulZoneMask & (0x00000001<<(0)))) { do{{ if (0) { (void)printf("%s(%d) - onFirmwareUpdate = true",__func__, 256); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 256 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 256 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 256, 0x01, "%s(%d) - onFirmwareUpdate = true",__func__, 256); } }; }while((void)0,0); delay(200); } }
            onFirmwareUpdate = false;
            CheckNewFirmware();
        }

        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s(%d) Sleeping for %d ms",__func__, 261, SleepInterval()); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 261 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 261 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 261, 0x01, "%s(%d) Sleeping for %d ms",__func__, 261, SleepInterval()); } }; }while((void)0,0); delay(200); } };
        delay(SleepInterval());
    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        { if ((dpCurSettings.ulZoneMask & (0x00000001<<(7)))) { do{{ if (0) { (void)printf("%s(%d) - Check for IoT Hub activities",__func__, 265); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 265 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 265 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 265, 0x01, "%s(%d) - Check for IoT Hub activities",__func__, 265); } }; }while((void)0,0); delay(200); } };
        delay(5000);
        DevKitMQTTClient_Check();
    }
}
