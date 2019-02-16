#include "Arduino.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "parson.h"
#include "config.h"

#include "ReadSensorData.h"
#include "AZ3166Leds.h"

#define DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING_NOT
#define DIAGNOSTIC_INFO_TWIN_PARSING_NOT
#define DIAGNOSTIC_INFO_SENSOR_DATA_NOT

static float temperature = 0;
static float humidity = 0;
static float pressure = 0;

bool CreateTelemetryMessage(char *payload, bool forceCreate, DEVICE_SETTINGS *pDeviceSettings)
{
    bool bReadHumidity = pDeviceSettings->enableHumidityReading;
    bool bReadPressure = pDeviceSettings->enablePressureReading;
    float t = ReadTemperature() + pDeviceSettings->temperatureCorrection;
    float h = ReadHumidity() + pDeviceSettings->humidityCorrection;
    float p = ReadPressure() + pDeviceSettings->pressureCorrection;
    float deltaT = abs(temperature - t);
    float deltaH = abs(humidity - h);
    float deltaP = abs(pressure - p);
    bool temperatureAlert = false;

#ifdef DIAGNOSTIC_INFO_SENSOR_DATA
    LogInfo("DIAGNOSTIC_INFO_SENSOR_DATA  deltaT = %.2f, deltaH = %.2f, deltaP = %.2f", deltaT, deltaH, deltaP);
#endif

    // Correct for sensor jitter
    bool temperatureChanged = (deltaT > pDeviceSettings->temperatureAccuracy) && (deltaT < pDeviceSettings->maxDeltaBetweenMeasurements);
    bool humidityChanged    = (deltaH > pDeviceSettings->humidityAccuracy)    && (deltaH < pDeviceSettings->maxDeltaBetweenMeasurements);
    bool pressureChanged    = (deltaP > pDeviceSettings->pressureAccuracy)    && (deltaP < pDeviceSettings->maxDeltaBetweenMeasurements);

    if (forceCreate) {
        temperatureChanged = humidityChanged = pressureChanged = true;
    }

#ifdef DIAGNOSTIC_INFO_SENSOR_DATA
    LogInfo("DIAGNOSTIC_INFO_SENSOR_DATA  temperatureChanged = %d, humidityChanged = %d, pressureChanged = %d", (int)temperatureChanged, (int)humidityChanged, (int)pressureChanged);
    LogInfo("DIAGNOSTIC_INFO_SENSOR_DATA  temperature = %.1f, humidity = %.1f, pressure = %.1f", temperature, humidity, pressure);
    LogInfo("DIAGNOSTIC_INFO_SENSOR_DATA  t = %.1f, h = %.1f, p = %.1f", t, h, p);
#endif

    if (temperatureChanged || humidityChanged || pressureChanged) {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        char *serialized_string = NULL;

        json_object_set_string(root_object, JSON_DEVICEID, DEVICE_ID);

        if (temperatureChanged) {
            temperature = t;
            json_object_set_number(root_object, JSON_TEMPERATURE, Round(temperature));
        }

        if(temperature > DEFAULT_TEMPERATURE_ALERT) {
            temperatureAlert = true;
        }

        if (humidityChanged) {
            humidity = h;
            if (bReadHumidity) {
                json_object_set_number(root_object, JSON_HUMIDITY, Round(humidity));
            }
        }

        if (pressureChanged) {
            pressure = p;
            if (bReadPressure) {
                json_object_set_number(root_object, JSON_PRESSURE, Round(pressure));
            }
        }

        ShowTelemetryData(temperature, humidity, pressure, pDeviceSettings);

        serialized_string = json_serialize_to_string_pretty(root_value);

        snprintf(payload, MESSAGE_MAX_LEN, "%s", serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    } else {
        *payload = '\0';
    }

    return temperatureAlert;
}

void CreateEventMsg(char *payload, IOTC_EVENT_TYPE eventType)
{
#ifdef DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING
    LogInfo("DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING  CreateEventMessage Invoked");
    delay(200);
#endif

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;

    json_object_set_string(root_object, JSON_DEVICEID, DEVICE_ID);
    if (eventType == WARNING_EVENT) {
        json_object_set_boolean(root_object, JSON_EVENT_WARNING, true);
    } else if (eventType == ERROR_EVENT) {
        json_object_set_boolean(root_object, JSON_EVENT_ERROR, true);
    }
    serialized_string = json_serialize_to_string_pretty(root_value);

    snprintf(payload, MESSAGE_MAX_LEN, "%s", serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

// const char *deviceFirmware = DEVICE_FIRMWARE_VERSION;
// const char *deviceLocation = DEVICE_LOCATION;
// const char *deviceId = DEVICE_ID;

const char *twinProperties="{\"Firmware\":\"%s\",\"Model\":\"%s\",\"Location\":\"%s\", \ 
                             \"measureInterval\":%d,\"sendInterval\":%d,\"warmingUpTime\":%d,\"sleepTime\":%d, \
                             \"temperatureAlert\":%d,\"temperatureAccuracy\":%1.1f,\"humidityAccuracy\":%1.1f,\"pressureAccuracy\":%1.1f, \
                             \"maxDeltaBetweenMeasurements\":%d, \
                             \"temperatureCorrection\":%1.1f,\"humidityCorrection\":%1.1f,\"pressureCorrection\":%1.1f, \
                             \"readHumidity\":%d, \"readPressure\":%d }";

bool SendDeviceInfo(DEVICE_SETTINGS *pDeviceSettings)
{
    char reportedProperties[2048];

    // Check for a configuration file to overrule the 'statically' set deviceFirmware, deviceLoaction and deviceId.
    // Open a file located on the device
    // char *devFW = (config ? config.devFW : deviceFirmware);
    char *devFW = DEVICE_FIRMWARE_VERSION;
    char *devLoc = DEVICE_LOCATION;
    char *devId = DEVICE_ID;
    // end check


    snprintf(reportedProperties, 2048, twinProperties, devFW, devId, devLoc, 
        pDeviceSettings->measureInterval, pDeviceSettings->sendInterval, pDeviceSettings->warmingUpTime, pDeviceSettings->dSmsec,
        pDeviceSettings->temperatureAlert, pDeviceSettings->temperatureAccuracy, pDeviceSettings->humidityAccuracy, pDeviceSettings->pressureAccuracy,
        pDeviceSettings->maxDeltaBetweenMeasurements,
        pDeviceSettings->temperatureCorrection, pDeviceSettings->humidityCorrection, pDeviceSettings->pressureCorrection,
        pDeviceSettings->enableHumidityReading ? 1 : 0, pDeviceSettings->enablePressureReading ? 1 : 0);
    return DevKitMQTTClient_ReportState(reportedProperties);
}

bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, DEVICE_SETTINGS *pDeviceSettings)
{
    JSON_Value *root_value;
    root_value = json_parse_string(message);
    
    bool sendAck = false;

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        LogError("parse %s failed", message);
        return sendAck;
    }

    JSON_Object *root_object = json_value_get_object(root_value);

    if (json_object_has_value(root_object, "desired")) {
        if (json_object_dothas_value(root_object,"desired.measureInterval.value")) {
            pDeviceSettings->measureInterval = (int)json_object_dotget_number(root_object, "desired.measureInterval.value");
            pDeviceSettings->mImsec = pDeviceSettings->measureInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"desired.sendInterval.value")) {
            pDeviceSettings->sendInterval = (int)json_object_dotget_number(root_object, "desired.sendInterval.value");
            pDeviceSettings->sImsec = pDeviceSettings->sendInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"desired.warmingUpTime.value")) {
            pDeviceSettings->warmingUpTime = (int)json_object_dotget_number(root_object, "desired.warmingUpTime.value");
            pDeviceSettings->wUTmsec = pDeviceSettings->warmingUpTime * 60000;
        }
        if (json_object_dothas_value(root_object,"desired.sleepTime.value")) {
            pDeviceSettings->dSmsec = (int)json_object_dotget_number(root_object, "desired.sleepTime.value");
        }
        if (json_object_dothas_value(root_object,"desired.temperatureAlert.value")) {
            pDeviceSettings->temperatureAlert = (int)json_object_dotget_number(root_object, "desired.temperatureAlert.value");
        }
        if (json_object_dothas_value(root_object,"desired.temperatureAccuracy.value")) {
            pDeviceSettings->temperatureAccuracy = json_object_dotget_number(root_object, "desired.temperatureAccuracy.value");
        }
        if (json_object_dothas_value(root_object,"desired.pressureAccuracy.value")) {
            pDeviceSettings->pressureAccuracy = json_object_dotget_number(root_object, "desired.pressureAccuracy.value");
        }
        if (json_object_dothas_value(root_object,"desired.humidityAccuracy.value")) {
            pDeviceSettings->humidityAccuracy = json_object_dotget_number(root_object, "desired.humidityAccuracy.value");
        }
        if (json_object_dothas_value(root_object,"desired.maxDeltaBetweenMeasurements.value")) {
            pDeviceSettings->maxDeltaBetweenMeasurements = (int)json_object_dotget_number(root_object, "desired.maxDeltaBetweenMeasurements.value");
        }
        if (json_object_dothas_value(root_object,"desired.temperatureCorrection.value")) {
            pDeviceSettings->temperatureCorrection = json_object_dotget_number(root_object, "desired.temperatureCorrection.value");
        }
        if (json_object_dothas_value(root_object,"desired.pressureCorrection.value")) {
            pDeviceSettings->pressureCorrection = json_object_dotget_number(root_object, "desired.pressureCorrection.value");
        }
        if (json_object_dothas_value(root_object,"desired.humidityCorrection.value")) {
            pDeviceSettings->humidityCorrection = json_object_dotget_number(root_object, "desired.humidityCorrection.value");
        }
        if (json_object_dothas_value(root_object,"desired.enableHumidityReading.value")) {
            pDeviceSettings->enableHumidityReading = json_object_dotget_boolean(root_object, "desired.enableHumidityReading.value");
        }
        if (json_object_dothas_value(root_object,"desired.enablePressureReading.value")) {
            pDeviceSettings->enablePressureReading = json_object_dotget_boolean(root_object, "desired.enablePressureReading.value");
        }
    } else {
        sendAck = true;
        if (json_object_dothas_value(root_object,"measureInterval.value")) {
            pDeviceSettings->measureInterval = (int)json_object_dotget_number(root_object, "measureInterval.value");
            pDeviceSettings->mImsec = pDeviceSettings->measureInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"sendInterval.value")) {
            pDeviceSettings->sendInterval = (int)json_object_dotget_number(root_object, "sendInterval.value");
            pDeviceSettings->sImsec = pDeviceSettings->sendInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"warmingUpTime.value")) {
            pDeviceSettings->warmingUpTime = (int)json_object_dotget_number(root_object, "warmingUpTime.value");
            pDeviceSettings->wUTmsec = pDeviceSettings->warmingUpTime * 60000;
        }
        if (json_object_dothas_value(root_object,"sleepTime.value")) {
            pDeviceSettings->dSmsec = (int)json_object_dotget_number(root_object, "sleepTime.value");
        }
        if (json_object_dothas_value(root_object,"temperatureAlert.value")) {
            pDeviceSettings->temperatureAlert = (int)json_object_dotget_number(root_object, "temperatureAlert.value");
        }
        if (json_object_dothas_value(root_object,"temperatureAccuracy.value")) {
            pDeviceSettings->temperatureAccuracy = json_object_dotget_number(root_object, "temperatureAccuracy.value");
        }
        if (json_object_dothas_value(root_object,"pressureAccuracy.value")) {
            pDeviceSettings->pressureAccuracy = json_object_dotget_number(root_object, "pressureAccuracy.value");
        }
        if (json_object_dothas_value(root_object,"humidityAccuracy.value")) {
            pDeviceSettings->humidityAccuracy = json_object_dotget_number(root_object, "humidityAccuracy.value");
        }
        if (json_object_dothas_value(root_object,"maxDeltaBetweenMeasurements.value")) {
            pDeviceSettings->maxDeltaBetweenMeasurements = (int)json_object_dotget_number(root_object, "maxDeltaBetweenMeasurements.value");
        }
        if (json_object_dothas_value(root_object,"temperatureCorrection.value")) {
            pDeviceSettings->temperatureCorrection = json_object_dotget_number(root_object, "temperatureCorrection.value");
        }
        if (json_object_dothas_value(root_object,"pressureCorrection.value")) {
            pDeviceSettings->pressureCorrection = json_object_dotget_number(root_object, "pressureCorrection.value");
        }
        if (json_object_dothas_value(root_object,"humidityCorrection.value")) {
            pDeviceSettings->humidityCorrection = json_object_dotget_number(root_object, "humidityCorrection.value");
        }
        if (json_object_dothas_value(root_object,"enableHumidityReading.value")) {
            pDeviceSettings->enableHumidityReading = json_object_dotget_boolean(root_object, "enableHumidityReading.value");
        }
        if (json_object_dothas_value(root_object,"enablePressureReading.value")) {
            pDeviceSettings->enablePressureReading = json_object_dotget_boolean(root_object, "enablePressureReading.value");
        }
    }

    json_value_free(root_value);

#ifdef DIAGNOSTIC_INFO_TWIN_PARSING
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  measureInterval = %d, sendInterval = %d, warmingUpTime = %d, delayTime = %d", pDeviceSettings->measureInterval, pDeviceSettings->sendInterval, pDeviceSettings->warmingUpTime, pDeviceSettings->dSmsec);
    delay(200);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  temperatureAccuracy = %f, pressureAccuracy = %f, humidityAccuracy = %f", pDeviceSettings->temperatureAccuracy, pDeviceSettings->pressureAccuracy, pDeviceSettings->humidityAccuracy);
    delay(200);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  temperatureAlert = %d, maxDeltaBetweenMeasurements = %d", pDeviceSettings->temperatureAlert, pDeviceSettings->maxDeltaBetweenMeasurements);
    delay(200);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  enableHumidityReading = %d, enablePressureReading = %d", pDeviceSettings->enableHumidityReading, pDeviceSettings->enablePressureReading);
#endif

    return sendAck;
}
