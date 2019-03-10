# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 2 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 3 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 4 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2

# 6 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2




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
# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
// To get started please visit https://microsoft.github.io/azure-iot-developer-kit/docs/projects/remote-monitoring/?utm_source=ArduinoExtension&utm_medium=ReleaseNote&utm_campaign=VSCode
# 5 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 6 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 7 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 8 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 9 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

//#include "UpdateFirmwareOTA.h"

# 13 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 14 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 15 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 17 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 18 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2

# 20 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2
# 21 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 2




static bool onReset = false;
static bool onMeasureNow = false;
static bool onFirmwareUpdate = false;

static bool messageSending = true;
static uint64_t send_interval_ms;
static uint64_t measure_interval_ms;
static uint64_t warming_up_interval_ms;
static uint64_t deviceStartTime = 0;

static bool reportProperties = false;

static DEVICE_SETTINGS deviceSettings { 10, 300, 500, 2, 0,
                                        10000, 300000, 120000, 500,
                                        40,
                                        0.2, 0.2, 0.5,
                                        5,
                                        0.0, 0.0, 0.0 };

void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    char payLoadString[length+1];
    snprintf(payLoadString, length, "%s", payLoad);





    char *temp = (char *)malloc(length + 1);
    if (temp == 
# 54 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
               __null
# 54 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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
# 97 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength)
{
    int result = 200;





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
    send_interval_ms = measure_interval_ms = warming_up_interval_ms = deviceStartTime = SystemTickCounterRead();
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

    if (nextMeasurementDue || nextMessageDue || onMeasureNow) {
        // Read Sensors ...
        char messagePayload[256];

        deviceSettings.upTime = (int)(SystemTickCounterRead() - deviceStartTime) / 1000;

        bool temperatureAlert = CreateTelemetryMessage(messagePayload, nextMessageDue || onMeasureNow, &deviceSettings);

        if (! suppressMessages) {

            // ... and send data if the sensor value(s) differ from the previous reading or when the device needs to give a sign of life.
            if (strlen(messagePayload) != 0) {
                char szUpTime[11];

                snprintf(szUpTime, 10, "%d", deviceSettings.upTime);
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

    } else if (reportProperties) {
        SendDeviceInfo(&deviceSettings);
        reportProperties = false;
    } else {
        DevKitMQTTClient_Check();
    }

    if (IsButtonClicked(USER_BUTTON_A)) {
        char messageEvt[256];
        CreateEventMsg(messageEvt, WARNING_EVENT);

        EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
        DevKitMQTTClient_SendEventInstance(message);
    }

    if (IsButtonClicked(USER_BUTTON_B)) {
        char messageEvt[256];
        CreateEventMsg(messageEvt, ERROR_EVENT);

        EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(messageEvt, MESSAGE);
        DevKitMQTTClient_SendEventInstance(message);
    }

    if (onReset) {
        onReset = false;
        __NVIC_SystemReset();
    }

    if (onFirmwareUpdate) {
        do{{ if (0) { (void)printf("Ready to call CheckNewFirmware"); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != 
# 218 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino" 3 4
       __null
# 218 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
       ) l(AZ_LOG_INFO, "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino", __func__, 218, 0x01, "Ready to call CheckNewFirmware"); } }; }while((void)0,0);
        onFirmwareUpdate = false;
        CheckNewFirmware();
    }




    delay(deviceSettings.dSmsec);

}
