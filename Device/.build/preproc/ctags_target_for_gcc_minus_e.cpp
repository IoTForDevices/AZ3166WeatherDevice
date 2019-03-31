# 1 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 1 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 2 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 3 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 4 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2

# 6 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2




static RGB_LED rgbLed;
static char displayBuffer[128];

void ShowTelemetryData(float temperature, float humidity, float pressure, DEVICE_SETTINGS *pDeviceSettings)
{
    snprintf(displayBuffer, 128, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
        f2s(temperature, 1), pDeviceSettings->enableHumidityReading ? f2s(humidity,1) : "-", pDeviceSettings->enablePressureReading ? f2s(pressure,1) : "-");
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
# 1 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
// To get started please visit https://microsoft.github.io/azure-iot-developer-kit/docs/projects/remote-monitoring/?utm_source=ArduinoExtension&utm_medium=ReleaseNote&utm_campaign=VSCode
# 5 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 6 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 7 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 8 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 9 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 11 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 12 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 13 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 15 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 16 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 18 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 19 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2





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

static bool reportProperties = false;

static DEVICE_SETTINGS reportedDeviceSettings { 10, 300, 500, 2, 0,
                                                10000, 300000, 120000, 500,
                                                40,
                                                0.2, 0.2, 0.5,
                                                5,
                                                0.0, 0.0, 0.0, 10 };
static DEVICE_PROPERTIES reportedDeviceProperties { 
# 48 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
                                                   __null
# 48 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
                                                       , 
# 48 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
                                                         __null
# 48 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
                                                             , 
# 48 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
                                                               __null 
# 48 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
                                                                    };
static DEVICE_SETTINGS desiredDeviceSettings;

void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    char payLoadString[length+1];
    snprintf(payLoadString, length, "%s", payLoad);


    do{{ if (0) { (void)printf("    TwinCallback - Payload: Length = %d", length); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 57 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
   __null
# 57 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 57, 0x01, "    TwinCallback - Payload: Length = %d", length); } }; }while((void)0,0);
    delay(200);

    char *temp = (char *)malloc(length + 1);
    if (temp == 
# 61 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
               __null
# 61 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
                   )
    {
        return;
    }
    memcpy(temp, payLoad, length);
    temp[length] = '\0';
    reportProperties = ParseTwinMessage(updateState, temp, &desiredDeviceSettings, &reportedDeviceSettings, &reportedDeviceProperties);


    do{{ if (0) { (void)printf("    reportProperties after ParseTwinMessage = %d", reportProperties); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 70 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
   __null
# 70 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 70, 0x01, "    reportProperties after ParseTwinMessage = %d", reportProperties); } }; }while((void)0,0);
    delay(200);


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
# 104 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength)
{
    int result = 200;


    do{{ if (0) { (void)printf("DeviceMethodCallback!"); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 109 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
   __null
# 109 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 109, 0x01, "DeviceMethodCallback!"); } }; }while((void)0,0);


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






        if (reportedDeviceSettings.enableMotionDetection) {
            // Check if the device is in motion or not and enable an alarm if so.
            bool motionDetected = MotionDetected(reportedDeviceSettings.motionSensitivity);

            if (motionDetected && nextMotionEventDue) {
                char messageEvt[256];
                CreateEventMsg(messageEvt, MOTION_EVENT);
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
                DevKitMQTTClient_SendEventInstance(message);

                motion_interval_ms = SystemTickCounterRead();




            }



        }

        if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
            // Read Sensors ...
            char messagePayload[256];

            reportedDeviceSettings.upTime = (int)(SystemTickCounterRead() - deviceStartTime) / 1000;

            bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMessageDue || onMeasureNow, &reportedDeviceSettings);

            if (! suppressMessages) {

                // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
                if (strlen(messagePayload) != 0) {
                    char szUpTime[11];

                    snprintf(szUpTime, 10, "%d", reportedDeviceSettings.upTime);
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

        // } else if (reportProperties) {
        //     SendDeviceInfo(&reportedDeviceSettings, &reportedDeviceProperties);
        //     reportProperties = false;
        } else {
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            onReset = false;
            __NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {

            do{{ if (0) { (void)printf("Ready to call CheckNewFirmware"); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 231 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
           __null
# 231 "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
           ) l(AZ_LOG_INFO, "c:\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 231, 0x01, "Ready to call CheckNewFirmware"); } }; }while((void)0,0);

            onFirmwareUpdate = false;
            CheckNewFirmware();
        }


        delay(2000);



    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        delay(5000);
        DevKitMQTTClient_Check();
    }
}
