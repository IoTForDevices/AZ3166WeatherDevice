#include "Arduino.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "parson.h"
#include "config.h"

#include "ReadSensorData.h"
#include "AZ3166Leds.h"
#include "IoTHubMessageHandling.h"

#define DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING_NOT
#define DIAGNOSTIC_INFO_TWIN_PARSING_NOT

static float temperature = 0;
static float humidity = 0;
static float pressure = 0;

bool CreateTelemetryMessage(int messageId, char *payload, bool forceCreate, DEVICE_SETTINGS *pDeviceSettings)
{
    float t = ReadTemperature() + pDeviceSettings->temperatureCorrection;
    float h = ReadHumidity() + pDeviceSettings->humidityCorrection;
    float p = ReadPressure() + pDeviceSettings->pressureCorrection;
    bool temperatureAlert = false;

    // Correct for sensor jitter
    bool temperatureChanged = (abs(temperature - t) > pDeviceSettings->temperatureAccuracy) && (abs(temperature - t) < pDeviceSettings->maxDeltaBetweenMeasurements);
    bool humidityChanged = (abs(humidity - h) > pDeviceSettings->humidityAccuracy) && (abs(humidity - t) < pDeviceSettings->maxDeltaBetweenMeasurements);
    bool pressureChanged = (abs(pressure - p) > pDeviceSettings->pressureAccuracy) && (abs(pressureChanged - t) < pDeviceSettings->maxDeltaBetweenMeasurements);

    if (forceCreate) {
        temperatureChanged = humidityChanged = pressureChanged = true;
    }

#ifdef DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING
    LogInfo("DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING  temperatureChanged = %d, humidityChanged = %d, pressureChanged = %d", (int)temperatureChanged, (int)humidityChanged, (int)pressureChanged);
    LogInfo("DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING  t = %.1f, h = %.1f, p = %.1f", t, h, p);
#endif

    if (temperatureChanged || humidityChanged || pressureChanged) {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        char *serialized_string = NULL;

        json_object_set_string(root_object, "deviceId", DEVICE_ID);
        json_object_set_number(root_object, "messageId", messageId);

        if (temperatureChanged) {
            temperature = t;
            json_object_set_number(root_object, "temperature", temperature);
        }

        if(temperature > DEFAULT_TEMPERATURE_ALERT) {
            temperatureAlert = true;
        }

        if (humidityChanged) {
            humidity = h;
            json_object_set_number(root_object, "humidity", humidity);
        }

        if (pressureChanged) {
            pressure = p;
            json_object_set_number(root_object, "pressure", pressure);
        }

        ShowTelemetryData(temperature, humidity, pressure);

        serialized_string = json_serialize_to_string_pretty(root_value);

        snprintf(payload, MESSAGE_MAX_LEN, "%s", serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    } else {
        *payload = '\0';
    }
    return temperatureAlert;
}

const char *deviceFirmware = "1.3.0";
const char *deviceLocation = DEVICE_LOCATION;
const char *deviceId = DEVICE_ID;
const double deviceLatitude = 52.176;
const double deviceLongitude = 4.65624;

const char *twinProperties="{\"Firmware\":\"%s\",\"Model\":\"%s\",\"Location\":\"%s\",\"Latitude\":%f,\"Longitude\":%f, \ 
                             \"measureInterval\":%d,\"sendInterval\":%d,\"warmingUpTime\":%d, \
                             \"temperatureAlert\":%d,\"temperatureAccuracy\":%1.1f,\"humidityAccuracy\":%1.1f,\"pressureAccuracy\":%1.1f, \
                             \"maxDeltaBetweenMeasurements\":%d, \
                             \"temperatureCorrection\":%1.1f,\"humidityCorrection\":%1.1f,\"pressureCorrection\":%1.1f }";

bool SendDeviceInfo(DEVICE_SETTINGS *pDeviceSettings)
{
    char reportedProperties[2048];
    snprintf(reportedProperties,2048, twinProperties, deviceFirmware, deviceId, deviceLocation, deviceLatitude, deviceLongitude, 
        pDeviceSettings->measureInterval, pDeviceSettings->sendInterval, pDeviceSettings->warmingUpTime,
        pDeviceSettings->temperatureAlert, pDeviceSettings->temperatureAccuracy, pDeviceSettings->humidityAccuracy, pDeviceSettings->pressureAccuracy,
        pDeviceSettings->maxDeltaBetweenMeasurements,
        pDeviceSettings->temperatureCorrection, pDeviceSettings->humidityCorrection, pDeviceSettings->pressureCorrection);
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
    }

    json_value_free(root_value);

#ifdef DIAGNOSTIC_INFO_TWIN_PARSING
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  measureInterval = %d, sendInterval = %d, warmingUpTime = %d", pDeviceSettings->measureInterval, pDeviceSettings->sendInterval, pDeviceSettings->warmingUpTime);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  temperatureAccuracy = %f, pressureAccuracy = %f, humidityAccuracy = %f", pDeviceSettings->temperatureAccuracy, pDeviceSettings->pressureAccuracy, pDeviceSettings->humidityAccuracy);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  temperatureAlert = %d, maxDeltaBetweenMeasurements = %d", pDeviceSettings->temperatureAlert, pDeviceSettings->maxDeltaBetweenMeasurements);
#endif

    return sendAck;
}
