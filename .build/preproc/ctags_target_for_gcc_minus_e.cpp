# 1 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
# 1 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
// To get started please visit https://microsoft.github.io/azure-iot-developer-kit/docs/projects/remote-monitoring/?utm_source=ArduinoExtension&utm_medium=ReleaseNote&utm_campaign=VSCode
# 5 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 6 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 7 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 8 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 9 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 10 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 11 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 12 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2

# 14 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 15 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 16 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2
# 17 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 2

static bool onReset = false;

static int messageCount = 1;
static bool messageSending = true;
static uint64_t send_interval_ms;
static uint64_t measure_interval_ms;
static uint64_t warming_up_interval_ms;
static bool reportProperties = false;

static DEVICE_SETTINGS deviceSettings { 10, 300, 2,
                                        10000, 300000, 120000,
                                        40,
                                        0.2, 0.2, 0.5,
                                        5,
                                        0.0, 0.0, 0.0 };

void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length){
    char payLoadString[length+1];
    snprintf(payLoadString, length, "%s", payLoad);
    do{{ LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 37 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 3 4
   __null
# 37 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, (strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '/') ? strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '/') + 1 : (strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '\\') ? strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '\\') + 1 : "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino")), __func__, 37, 0x01, "    Payload: %s", payLoadString); }; }while((void)0,0);

    char *temp = (char *)malloc(length + 1);
    if (temp == 
# 40 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 3 4
               __null
# 40 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
                   )
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

    do{{ LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 81 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino" 3 4
   __null
# 81 "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino"
   ) l(AZ_LOG_INFO, (strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '/') ? strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '/') + 1 : (strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '\\') ? strrchr("c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino", '\\') + 1 : "c:\\Repo\\AZ3166WeatherDevice\\AZ3166WeatherDevice.ino")), __func__, 81, 0x01, "DeviceMethodCallback!"); }; }while((void)0,0);
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
        // Read Sensors
        char messagePayload[256];

        bool temperatureAlert = CreateTelemetryMessage(messageCount, messagePayload, nextMessageDue, &deviceSettings);

        if (! suppressMessages) {
            if (nextMessageDue)
            {
                send_interval_ms = SystemTickCounterRead(); // Send at least one complete message per nextMessageDue interval
            }

            // And send data if the sensor value(s) differ from the previous reading
            if (strlen(messagePayload) != 0) {
                EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messagePayload, MESSAGE);
                DevKitMQTTClient_Event_AddProp(message, "temperatureAlert", temperatureAlert ? "true" : "false");
                DevKitMQTTClient_SendEventInstance(message);
                messageCount++;
            }
        }

        measure_interval_ms = SystemTickCounterRead(); // reset regardless of message send after each sensor reading
    } else if (reportProperties) {
        SendDeviceInfo(&deviceSettings);
        reportProperties = false;
    } else {
        DevKitMQTTClient_Check();
    }
    delay(500);
}
