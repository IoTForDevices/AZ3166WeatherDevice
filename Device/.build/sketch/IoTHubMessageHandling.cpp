#include "Arduino.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "parson.h"
#include "config.h"

#include "ReadSensorData.h"
#include "AZ3166Leds.h"
//#include "DebugZones.h"

#define DIAGNOSTIC_INFO_IOTHUBMESSAGEHANDLING_NOT
#define DIAGNOSTIC_INFO_TWIN_PARSING
#define DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
#define DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
#define DIAGNOSTIC_INFO_TWIN_PARSING_RAW_DATA_NOT
#define DIAGNOSTIC_INFO_SENSOR_DATA_NOT

static float temperature = 0;
static float humidity = 0;
static float pressure = 0;
static bool  initialDeviceTwinReportReceived = false;

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
    if (eventType == MOTION_EVENT) {
        json_object_set_string(root_object, JSON_EVENT_MOTION, "true");
//      json_object_set_boolean(root_object, JSON_EVENT_MOTION, true);
//      json_object_set_number(root_object, JSON_EVENT_MOTION, 1);
    }
    serialized_string = json_serialize_to_string_pretty(root_value);

    snprintf(payload, MESSAGE_MAX_LEN, "%s", serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

const char *twinProperties="{\"Firmware\":\"%s\",\"Model\":\"%s\",\"Location\":\"%s\", \ 
                             \"measureInterval\":%d,\"sendInterval\":%d,\"warmingUpTime\":%d,\"sleepTime\":%d,\"motionDetectionInterval\":%d, \
                             \"temperatureAlert\":%d,\"temperatureAccuracy\":%1.1f,\"humidityAccuracy\":%1.1f,\"pressureAccuracy\":%1.1f, \
                             \"maxDeltaBetweenMeasurements\":%d, \
                             \"temperatureCorrection\":%1.1f,\"humidityCorrection\":%1.1f,\"pressureCorrection\":%1.1f, \"motionSensitivity\":%d, \
                             \"readHumidity\":%s, \"readPressure\":%s, \"enableMotionDetection\":%s }";

bool SendDeviceInfo(DEVICE_SETTINGS *pDeviceSettings, DEVICE_PROPERTIES *pDeviceProperties)
{
    char reportedProperties[2048];

#ifdef DIAGNOSTIC_INFO_TWIN_PARSING
    LogInfo("SendDeviceInfo to update Reported Properties.");
    delay(200);
#endif

    snprintf(reportedProperties, 2048, twinProperties, pDeviceProperties->pszCurrentFwVersion, pDeviceProperties->pszDeviceModel, pDeviceProperties->pszLocation, 
        pDeviceSettings->measureInterval, pDeviceSettings->sendInterval, pDeviceSettings->warmingUpTime, pDeviceSettings->dSmsec, pDeviceSettings->motionDetectionInterval,
        pDeviceSettings->temperatureAlert, pDeviceSettings->temperatureAccuracy, pDeviceSettings->humidityAccuracy, pDeviceSettings->pressureAccuracy,
        pDeviceSettings->maxDeltaBetweenMeasurements,
        pDeviceSettings->temperatureCorrection, pDeviceSettings->humidityCorrection, pDeviceSettings->pressureCorrection, pDeviceSettings->motionSensitivity,
        pDeviceSettings->enableHumidityReading ? "true" : "false", pDeviceSettings->enablePressureReading ? "true" : "false", pDeviceSettings->enableMotionDetection ? "true" : "false");
    return DevKitMQTTClient_ReportState(reportedProperties);
}

bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, DEVICE_SETTINGS *pDesiredDeviceSettings, DEVICE_SETTINGS *pReportedDeviceSettings, DEVICE_PROPERTIES *pReportedDeviceProperties)
{
    JSON_Value *root_value;
    root_value = json_parse_string(message);

 #ifdef DIAGNOSTIC_INFO_TWIN_PARSING_RAW_DATA
    for (int i = 0; i < strlen(message); i = i + 80)
    {
        char szTmp[81];
        strncpy(szTmp, message + i, 80);
        szTmp[80] = '\0';
        LogInfo("%s", szTmp);
        delay(200);
    }
#endif
   
    bool sendAck = false;

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        LogError("parse %s failed", message);
        return sendAck;
    }

    sendAck = true;
    JSON_Object *root_object = json_value_get_object(root_value);

    if (json_object_has_value(root_object, "desired")) {
        // Device Twin desired and reported properties when connected to the IoT Hub
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired or reported properties found!");
            delay(200);
#endif            
        if (json_object_dothas_value(root_object,"desired.measureInterval.value")) {
            pDesiredDeviceSettings->measureInterval = json_object_dotget_number(root_object, "desired.measureInterval.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.measureInterval.value = %d", pDesiredDeviceSettings->measureInterval);
            delay(200);
#endif            
            pDesiredDeviceSettings->mImsec = pDesiredDeviceSettings->measureInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"desired.sendInterval.value")) {
            pDesiredDeviceSettings->sendInterval = json_object_dotget_number(root_object, "desired.sendInterval.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.sendInterval.value = %d", pDesiredDeviceSettings->sendInterval);
            delay(200);
#endif            
            pDesiredDeviceSettings->sImsec = pDesiredDeviceSettings->sendInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"desired.warmingUpTime.value")) {
            pDesiredDeviceSettings->warmingUpTime = json_object_dotget_number(root_object, "desired.warmingUpTime.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.warmingUpTime.value = %d", pDesiredDeviceSettings->warmingUpTime);
            delay(200);
#endif            
            pDesiredDeviceSettings->wUTmsec = pDesiredDeviceSettings->warmingUpTime * 60000;
        }
        if (json_object_dothas_value(root_object,"desired.motionDetectionInterval.value")) {
            pDesiredDeviceSettings->motionDetectionInterval = json_object_dotget_number(root_object, "desired.motionDetectionInterval.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.motionDetectionInterval.value = %d", pDesiredDeviceSettings->motionDetectionInterval);
            delay(200);
#endif            
            pDesiredDeviceSettings->motionInMsec = pDesiredDeviceSettings->motionDetectionInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"desired.sleepTime.value")) {
            pDesiredDeviceSettings->dSmsec = json_object_dotget_number(root_object, "desired.sleepTime.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.sleepTime.value = %d", pDesiredDeviceSettings->dSmsec);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.temperatureAlert.value")) {
            pDesiredDeviceSettings->temperatureAlert = json_object_dotget_number(root_object, "desired.temperatureAlert.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.temperatureAlert.value = %d", pDesiredDeviceSettings->temperatureAlert);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.temperatureAccuracy.value")) {
            pDesiredDeviceSettings->temperatureAccuracy = json_object_dotget_number(root_object, "desired.temperatureAccuracy.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.temperatureAccuracy.value = %.1f", pDesiredDeviceSettings->temperatureAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.pressureAccuracy.value")) {
            pDesiredDeviceSettings->pressureAccuracy = json_object_dotget_number(root_object, "desired.pressureAccuracy.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.pressureAccuracy.value = %.1f", pDesiredDeviceSettings->pressureAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.humidityAccuracy.value")) {
            pDesiredDeviceSettings->humidityAccuracy = json_object_dotget_number(root_object, "desired.humidityAccuracy.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.humidityAccuracy.value = %.1f", pDesiredDeviceSettings->humidityAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.maxDeltaBetweenMeasurements.value")) {
            pDesiredDeviceSettings->maxDeltaBetweenMeasurements = json_object_dotget_number(root_object, "desired.maxDeltaBetweenMeasurements.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.maxDeltaBetweenMeasurements.value = %d", pDesiredDeviceSettings->maxDeltaBetweenMeasurements);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.temperatureCorrection.value")) {
            pDesiredDeviceSettings->temperatureCorrection = json_object_dotget_number(root_object, "desired.temperatureCorrection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.temperatureCorrection.value = %.1f", pDesiredDeviceSettings->temperatureCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.pressureCorrection.value")) {
            pDesiredDeviceSettings->pressureCorrection = json_object_dotget_number(root_object, "desired.pressureCorrection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.pressureCorrection.value = %.1f", pDesiredDeviceSettings->pressureCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.humidityCorrection.value")) {
            pDesiredDeviceSettings->humidityCorrection = json_object_dotget_number(root_object, "desired.humidityCorrection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.humidityCorrection.value = %.1f", pDesiredDeviceSettings->humidityCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.motionSensitivity.value")) {
            pDesiredDeviceSettings->motionSensitivity = json_object_dotget_number(root_object, "desired.motionSensitivity.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.motionSensitivity.value = %d", pDesiredDeviceSettings->motionSensitivity);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.enableHumidityReading.value")) {
            pDesiredDeviceSettings->enableHumidityReading = json_object_dotget_boolean(root_object, "desired.enableHumidityReading.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.enableHumidityReading.value = %d", pDesiredDeviceSettings->enableHumidityReading);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.enablePressureReading.value")) {
            pDesiredDeviceSettings->enablePressureReading = json_object_dotget_boolean(root_object, "desired.enablePressureReading.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.enablePressureReading.value = %d", pDesiredDeviceSettings->enablePressureReading);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"desired.enableMotionDetection.value")) {
            pDesiredDeviceSettings->enableMotionDetection = json_object_dotget_boolean(root_object, "desired.enableMotionDetection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("desired.enableMotionDetection.value = %d", pDesiredDeviceSettings->enableMotionDetection);
            delay(200);
#endif            
        }

        if (json_object_dothas_value(root_object, "reported.measureInterval")) {
            pReportedDeviceSettings->measureInterval = json_object_dotget_number(root_object, "reported.measureInterval");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.measureInterval = %d", pReportedDeviceSettings->measureInterval);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.sendInterval")) {
            pReportedDeviceSettings->sendInterval = json_object_dotget_number(root_object, "reported.sendInterval");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.sendInterval = %d", pReportedDeviceSettings->sendInterval);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.warmingUpTime")) {
            pReportedDeviceSettings->warmingUpTime = json_object_dotget_number(root_object, "reported.warmingUpTime");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.warmingUpTime = %d", pReportedDeviceSettings->warmingUpTime);
            delay(200);
#endif            
            pReportedDeviceSettings->wUTmsec = pReportedDeviceSettings->warmingUpTime * 60000;
        }
        if (json_object_dothas_value(root_object,"reported.sleepTime")) {
            pReportedDeviceSettings->dSmsec = json_object_dotget_number(root_object, "reported.sleepTime");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.sleepTime = %d", pReportedDeviceSettings->dSmsec);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.temperatureAlert")) {
            pReportedDeviceSettings->temperatureAlert = json_object_dotget_number(root_object, "reported.temperatureAlert");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.temperatureAlert = %d", pReportedDeviceSettings->temperatureAlert);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.temperatureAccuracy")) {
            pReportedDeviceSettings->temperatureAccuracy = json_object_dotget_number(root_object, "reported.temperatureAccuracy");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.temperatureAccuracy = %.1f", pReportedDeviceSettings->temperatureAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.humidityAccuracy")) {
            pReportedDeviceSettings->humidityAccuracy = json_object_dotget_number(root_object, "reported.humidityAccuracy");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.humidityAccuracy = %.1f", pReportedDeviceSettings->humidityAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.pressureAccuracy")) {
            pReportedDeviceSettings->pressureAccuracy = json_object_dotget_number(root_object, "reported.pressureAccuracy");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.pressureAccuracy = %.1f", pReportedDeviceSettings->pressureAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.maxDeltaBetweenMeasurements")) {
            pReportedDeviceSettings->maxDeltaBetweenMeasurements = json_object_dotget_number(root_object, "reported.maxDeltaBetweenMeasurements");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.maxDeltaBetweenMeasurements = %d", pReportedDeviceSettings->maxDeltaBetweenMeasurements);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.temperatureCorrection")) {
            pReportedDeviceSettings->temperatureCorrection = json_object_dotget_number(root_object, "reported.temperatureCorrection");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.temperatureCorrection = %.1f", pReportedDeviceSettings->temperatureCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.humidityCorrection")) {
            pReportedDeviceSettings->humidityCorrection = json_object_dotget_number(root_object, "reported.humidityCorrection");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.humidityCorrection = %.1f", pReportedDeviceSettings->humidityCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.pressureCorrection")) {
            pReportedDeviceSettings->pressureCorrection = json_object_dotget_number(root_object, "reported.pressureCorrection");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.pressureCorrection = %.1f", pReportedDeviceSettings->pressureCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.readHumidity")) {
            pReportedDeviceSettings->enableHumidityReading = json_object_dotget_boolean(root_object, "reported.readHumidity");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.readHumidity = %d", pReportedDeviceSettings->enableHumidityReading);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.readPressure")) {
            pReportedDeviceSettings->enablePressureReading = json_object_dotget_boolean(root_object, "reported.readPressure");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.readPressure = %d", pReportedDeviceSettings->enablePressureReading);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.enableMotionDetection")) {
            pReportedDeviceSettings->enableMotionDetection = json_object_dotget_boolean(root_object, "reported.enableMotionDetection");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.enableMotionDetection = %d", pReportedDeviceSettings->enableMotionDetection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.motionSensitivity")) {
            pReportedDeviceSettings->motionSensitivity = json_object_dotget_number(root_object, "reported.motionSensitivity");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.motionSensitivity = %d", pReportedDeviceSettings->motionSensitivity);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"reported.motionDetectionInterval")) {
            pReportedDeviceSettings->motionDetectionInterval = json_object_dotget_number(root_object, "reported.motionDetectionInterval");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.motionDetectionInterval = %d", pReportedDeviceSettings->motionDetectionInterval);
            delay(200);
#endif            
            pReportedDeviceSettings->motionInMsec = pReportedDeviceSettings->motionDetectionInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"reported.firmware.currentFwVersion")) {
            char *pszVersion = (char *)json_object_dotget_string(root_object, "reported.firmware.currentFwVersion");
            if (pReportedDeviceProperties->pszCurrentFwVersion != NULL) {
                free(pReportedDeviceProperties->pszCurrentFwVersion);
                pReportedDeviceProperties->pszCurrentFwVersion = NULL;
            }
            int fwVersionLength = strlen(pszVersion);
            pReportedDeviceProperties->pszCurrentFwVersion = (char *)malloc(fwVersionLength + 1);
            snprintf(pReportedDeviceProperties->pszCurrentFwVersion, fwVersionLength + 1, "%s", pszVersion);
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.firmware.currentFwVersion = %s", pReportedDeviceProperties->pszCurrentFwVersion);
            delay(200);
#endif
        }
        if (json_object_dothas_value(root_object,"reported.Model")) {
            char *pszModel = (char *)json_object_dotget_string(root_object, "reported.Model");
            if (pReportedDeviceProperties->pszDeviceModel != NULL) {
                free(pReportedDeviceProperties->pszDeviceModel);
                pReportedDeviceProperties->pszDeviceModel = NULL;
            }
            int nModelLength = strlen(pszModel);
            pReportedDeviceProperties->pszDeviceModel = (char *)malloc(nModelLength + 1);
            snprintf(pReportedDeviceProperties->pszDeviceModel, nModelLength + 1, "%s", pszModel);
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.Model = %s", pReportedDeviceProperties->pszDeviceModel);
            delay(200);
#endif
        }
        if (json_object_dothas_value(root_object,"reported.Location")) {
            char *pszLocation = (char *)json_object_dotget_string(root_object, "reported.Location");
            if (pReportedDeviceProperties->pszLocation != NULL) {
                free(pReportedDeviceProperties->pszLocation);
                pReportedDeviceProperties->pszLocation = NULL;
            }
            int nLocationLength = strlen(pszLocation);
            pReportedDeviceProperties->pszLocation = (char *)malloc(nLocationLength + 1);
            snprintf(pReportedDeviceProperties->pszLocation, nLocationLength + 1, "%s", pszLocation);
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_REPORTED
            LogInfo("reported.Location = %s", pReportedDeviceProperties->pszLocation);
            delay(200);
#endif
        }
        initialDeviceTwinReportReceived = true;
    } else {        // We are informed about a change in one of the desired properties
        if (json_object_dothas_value(root_object,"measureInterval.value")) {
            pDesiredDeviceSettings->measureInterval = (int)json_object_dotget_number(root_object, "measureInterval.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("measureInterval.value = %d", pDesiredDeviceSettings->measureInterval);
            delay(200);
#endif            
            pDesiredDeviceSettings->mImsec = pDesiredDeviceSettings->measureInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"sendInterval.value")) {
            pDesiredDeviceSettings->sendInterval = (int)json_object_dotget_number(root_object, "sendInterval.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("sendInterval.value = %d", pDesiredDeviceSettings->sendInterval);
            delay(200);
#endif            
            pDesiredDeviceSettings->sImsec = pDesiredDeviceSettings->sendInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"warmingUpTime.value")) {
            pDesiredDeviceSettings->warmingUpTime = (int)json_object_dotget_number(root_object, "warmingUpTime.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("warmingUpTime.value = %d", pDesiredDeviceSettings->warmingUpTime);
            delay(200);
#endif            
            pDesiredDeviceSettings->wUTmsec = pDesiredDeviceSettings->warmingUpTime * 60000;
        }
        if (json_object_dothas_value(root_object,"motionDetectionInterval.value")) {
            pDesiredDeviceSettings->motionDetectionInterval = (int)json_object_dotget_number(root_object, "motionDetectionInterval.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("motionDetectionInterval.value = %d", pDesiredDeviceSettings->motionDetectionInterval);
            delay(200);
#endif            
            pDesiredDeviceSettings->motionInMsec = pDesiredDeviceSettings->motionDetectionInterval * 1000;
        }
        if (json_object_dothas_value(root_object,"sleepTime.value")) {
            pDesiredDeviceSettings->dSmsec = (int)json_object_dotget_number(root_object, "sleepTime.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("sleepTime.value = %d", pDesiredDeviceSettings->dSmsec);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"temperatureAlert.value")) {
            pDesiredDeviceSettings->temperatureAlert = (int)json_object_dotget_number(root_object, "temperatureAlert.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("temperatureAlert.value = %d", pDesiredDeviceSettings->temperatureAlert);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"temperatureAccuracy.value")) {
            pDesiredDeviceSettings->temperatureAccuracy = json_object_dotget_number(root_object, "temperatureAccuracy.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("temperatureAccuracy.value = %d", pDesiredDeviceSettings->temperatureAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"pressureAccuracy.value")) {
            pDesiredDeviceSettings->pressureAccuracy = json_object_dotget_number(root_object, "pressureAccuracy.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("pressureAccuracy.value = %d", pDesiredDeviceSettings->pressureAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"humidityAccuracy.value")) {
            pDesiredDeviceSettings->humidityAccuracy = json_object_dotget_number(root_object, "humidityAccuracy.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("humidityAccuracy.value = %d", pDesiredDeviceSettings->humidityAccuracy);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"maxDeltaBetweenMeasurements.value")) {
            pDesiredDeviceSettings->maxDeltaBetweenMeasurements = (int)json_object_dotget_number(root_object, "maxDeltaBetweenMeasurements.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("maxDeltaBetweenMeasurements.value = %d", pDesiredDeviceSettings->maxDeltaBetweenMeasurements);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"temperatureCorrection.value")) {
            pDesiredDeviceSettings->temperatureCorrection = json_object_dotget_number(root_object, "temperatureCorrection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("temperatureCorrection.value = %d", pDesiredDeviceSettings->temperatureCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"pressureCorrection.value")) {
            pDesiredDeviceSettings->pressureCorrection = json_object_dotget_number(root_object, "pressureCorrection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("pressureCorrection.value = %d", pDesiredDeviceSettings->pressureCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"humidityCorrection.value")) {
            pDesiredDeviceSettings->humidityCorrection = json_object_dotget_number(root_object, "humidityCorrection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("humidityCorrection.value = %d", pDesiredDeviceSettings->humidityCorrection);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"motionSensitivity.value")) {
            pDesiredDeviceSettings->motionSensitivity = json_object_dotget_number(root_object, "motionSensitivity.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("motionSensitivity.value = %d", pDesiredDeviceSettings->motionSensitivity);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"enableHumidityReading.value")) {
            pDesiredDeviceSettings->enableHumidityReading = json_object_dotget_boolean(root_object, "enableHumidityReading.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("enableHumidityReading.value = %d", pDesiredDeviceSettings->enableHumidityReading);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"enablePressureReading.value")) {
            pDesiredDeviceSettings->enablePressureReading = json_object_dotget_boolean(root_object, "enablePressureReading.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("denablePressureReading.value = %d", pDesiredDeviceSettings->enablePressureReading);
            delay(200);
#endif            
        }
        if (json_object_dothas_value(root_object,"enableMotionDetection.value")) {
            pDesiredDeviceSettings->enableMotionDetection = json_object_dotget_boolean(root_object, "enableMotionDetection.value");
#ifdef DIAGNOSTIC_INFO_TWIN_PARSING_DESIRED
            LogInfo("enableMotionDetection.value = %d", pDesiredDeviceSettings->enableMotionDetection);
            delay(200);
#endif            
        }
    }

    json_value_free(root_value);

#ifdef DIAGNOSTIC_INFO_TWIN_PARSING
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  measureInterval = %d, sendInterval = %d, warmingUpTime = %d, delayTime = %d", pDesiredDeviceSettings->measureInterval, pDesiredDeviceSettings->sendInterval, pDesiredDeviceSettings->warmingUpTime, pDesiredDeviceSettings->dSmsec);
    delay(200);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  temperatureAccuracy = %f, pressureAccuracy = %f, humidityAccuracy = %f", pDesiredDeviceSettings->temperatureAccuracy, pDesiredDeviceSettings->pressureAccuracy, pDesiredDeviceSettings->humidityAccuracy);
    delay(200);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  temperatureAlert = %d, maxDeltaBetweenMeasurements = %d", pDesiredDeviceSettings->temperatureAlert, pDesiredDeviceSettings->maxDeltaBetweenMeasurements);
    delay(200);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  motionSensitivity = %d, motionDetectionInterval = %d", pDesiredDeviceSettings->motionSensitivity, pDesiredDeviceSettings->motionDetectionInterval);
    delay(200);
    LogInfo("DIAGNOSTIC_INFO_TWIN_PARSING  enableHumidityReading = %d, enablePressureReading = %d, enableMotionDetection = %d", pDesiredDeviceSettings->enableHumidityReading, pDesiredDeviceSettings->enablePressureReading, pDesiredDeviceSettings->enableMotionDetection);
#endif

    return sendAck;
}

bool InitialDeviceTwinDesiredReceived()
{
    return initialDeviceTwinReportReceived;
}
