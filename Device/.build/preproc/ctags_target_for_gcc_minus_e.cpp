# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 1 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
# 2 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 3 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2
# 4 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2

# 6 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp" 2




static RGB_LED rgbLed;
static char displayBuffer[128];

/**************************************************************************************************************

 * ShowTelemetryData

 * 

 * Show Humidity, Temperature and Pressure readings on the on-board display.

 **************************************************************************************************************/
# 18 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void ShowTelemetryData(float temperature, float humidity, float pressure, bool showHumidity, bool showPressure)
{
    snprintf(displayBuffer, 128, "Environment\r\nTemp:%s C\r\nHumidity:%s%%\r\nPressure:%s\r\n",
        f2s(temperature, 1), showHumidity ? f2s(humidity,1) : "-", showPressure ? f2s(pressure,1) : "-");
    Screen.print(displayBuffer);
}

/**************************************************************************************************************

 * BlinkLED

 * 

 * Turn on the LED for half a second with a not too bright red color.

 **************************************************************************************************************/
# 30 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
void BlinkLED()
{
    rgbLed.turnOff();
    rgbLed.setColor(32, 0, 0);
    delay(500);
    rgbLed.turnOff();
}

/**************************************************************************************************************

 * BlinkSendConfirmation

 * 

 * Turn on the LED for half a second with a not too bright blue color.

 **************************************************************************************************************/
# 43 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166Leds.cpp"
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
# 37 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"

# 37 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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
# 64 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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
# 85 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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
# 101 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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
# 113 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int length, unsigned char **response, int *responseLength)
{
    int result = 200;

    ;;

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
# 139 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int length)
{
    ;
    ParseTwinMessage(updateState, (const char *)payLoad, length);
}

/********************************************************************************************************

 * Int64ToString is a helper function that allows easy display / logging of long integers. These long integers

 * are mainly used in this application to find out when timers are expired in the main loop, after which

 * some action needs to be taken.

 * 

 * NOTE: This function is only available when LOGGING is enabled.

 *******************************************************************************************************/






/********************************************************************************************************

 * setup is the default initialisation function for Arduino based devices. It will be executed once

 * after (re)booting the system. During initialization we enable WiFi, connect to the IoT Hub and define

 * the callback functions for direct method execution and desired twin updates.

 *******************************************************************************************************/
# 169 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
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
# 191 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
void loop()
{
    if (InitialDeviceTwinDesiredReceived()) {
        uint64_t tickCount = SystemTickCounterRead();
# 208 "c:\\Source\\Repo\\AZ3166WeatherDevice\\Device\\AZ3166WeatherDevice.ino"
        nextMeasurementDue = (tickCount - measure_interval_ms) >= MeasurementInterval();
        ;;
        nextMessageDue = (tickCount - send_interval_ms) >= SendInterval();
        ;;
        nextMotionEventDue = (tickCount - motion_interval_ms) >= MotionInterval();
        ;;

        if (! firstMessageSend) {
            suppressMessages = (tickCount - warming_up_interval_ms) < WarmingUpTime();
            ;;

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
                ;;
            }

            ;;
        }

        if (UpdateReportedValues()) {
            ;;
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
                // Send reported property to indicate that we just read all sensor values.

            }

        } else {
            ;;
            DevKitMQTTClient_Check();
        }

        if (onReset) {
            ;
            onReset = false;
            __NVIC_SystemReset();
        }

        if (onFirmwareUpdate) {
            ;
            onFirmwareUpdate = false;
            CheckNewFirmware();
        }

        if (onResetFWVersion) {
            ;
            onResetFWVersion = false;
            CheckResetFirmwareInfo();
        }

        ;;
        delay(SleepInterval());
    } else {
        // No initial desired twin values received so assume that deviceSettings does not yet contain the right value
        ;;
        delay(5000);
        DevKitMQTTClient_Check();
    }
}
