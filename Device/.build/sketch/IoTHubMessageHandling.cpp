#include "Arduino.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "parson.h"
#include "config.h"

#include "ReadSensorData.h"
#include "AZ3166Leds.h"
#include "IoTHubMessageHandling.h"
#include "DebugZones.h"

static bool initialDeviceTwinReportReceived = false;
static bool hasReportedValues = false;
static bool isInitialized = false;
static bool updateReportedValues = false;

DEVICE_ENTRIES deviceTwinEntries[] = {
    { VALUE_INT,    "measureInterval" },
    { VALUE_INT,    "sendInterval" },
    { VALUE_INT,    "warmingUpTime" },
    { VALUE_INT,    "motionDetectionInterval" },
    { VALUE_INT,    "sleepTime" },
    { VALUE_INT,    "temperatureAlert" },
    { VALUE_FLOAT,  "temperatureAccuracy" },
    { VALUE_FLOAT,  "humidityAccuracy" },
    { VALUE_FLOAT,  "pressureAccuracy" },
    { VALUE_FLOAT,  "maxDeltaBetweenMeasurements" },
    { VALUE_FLOAT,  "temperatureCorrection" },
    { VALUE_FLOAT,  "humidityCorrection" },
    { VALUE_FLOAT,  "pressureCorrection" },
    { VALUE_BOOL,   "enableHumidityReading" },
    { VALUE_BOOL,   "enablePressureReading" },
    { VALUE_BOOL,   "enableMotionDetection" },
    { VALUE_INT,    "motionSensitivity" },
    { VALUE_STRING, "firmware.currentFwVersion" },
    { VALUE_STRING, "Model" },
    { VALUE_STRING, "Location" },
    { VALUE_INT,    "actualDebugMask"}
};

DEVICE_ENTRIES telemetryEntries[] = {
    { VALUE_FLOAT, "temperature" },
    { VALUE_FLOAT, "humidity" },
    { VALUE_FLOAT, "pressure" },
    { VALUE_INT,   "upTime" }
};

const int iExpectedValues = sizeof(deviceTwinEntries)/sizeof(deviceTwinEntries[0]);
const int iTelemetryValues = sizeof(telemetryEntries)/sizeof(telemetryEntries[0]);
DEVICE_DATA desiredTwinValues[iExpectedValues];         // These are the values that are requested from the IOTC application
DEVICE_DATA reportedTwinValues[iExpectedValues];        // These are the values used by the device application
DEVICE_PROPERTIES reportedTwinProperties[3];            // These are the reported properties from the IOTC application
DEVICE_DATA telemetryValues[iTelemetryValues];

void InitializeReportedTwinValues()
{
    reportedTwinValues[IDX_MEASUREINTERVAL].iValue = DEFAULT_MEASURE_INTERVAL; 
    reportedTwinValues[IDX_SENDINTERVAL].iValue = DEFAULT_SEND_INTERVAL; 
    reportedTwinValues[IDX_WARMINGUPTIME].iValue = DEFAULT_WARMING_UP_TIME; 
    reportedTwinValues[IDX_MOTIONDETECTIONINTERVAL].iValue = DEFAULT_MOTION_DETECTION_INTERVAL; 
    reportedTwinValues[IDX_SLEEPTIME].iValue = DEFAULT_SLEEP_INTERVAL; 
    reportedTwinValues[IDX_TEMPERATUREALERT].iValue =  DEFAULT_TEMPERATURE_ALERT;
    reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue = DEFAULT_TEMPERATURE_ACCURACY; 
    reportedTwinValues[IDX_HUMIDITYACCURACY].fValue = DEFAULT_HUMIDITY_ACCURACY; 
    reportedTwinValues[IDX_PRESSUREACCURACY].fValue = DEFAULT_PRESSURE_ACCURACY;
    reportedTwinValues[IDX_MAXDELTABETWEENMEASUREMENTS].iValue = DEFAULT_MAX_DELTA_BETWEEN_MEASUREMENTS;
    reportedTwinValues[IDX_TEMPERATURECORRECTION].fValue = DEFAULT_TEMPERATURE_CORRECTION;
    reportedTwinValues[IDX_HUMIDITYCORRECTION].fValue = DEFAULT_HUMIDITY_CORRECTION;
    reportedTwinValues[IDX_PRESSURECORRECTION].fValue = DEFAULT_PRESSURE_CORRECTION;
    reportedTwinValues[IDX_READHUMIDITY].bValue = DEFAULT_READ_HUMIDITY;
    reportedTwinValues[IDX_READPRESSURE].bValue = DEFAULT_READ_PRESSURE;
    reportedTwinValues[IDX_ENABLEMOTIONDETECTION].bValue = DEFAULT_DETECT_MOTION;
    reportedTwinValues[IDX_MOTIONSENSITIVITY].iValue = DEFAULT_MOTION_SENSITIVITY;
    reportedTwinValues[IDX_DEBUGMASK].iValue = DEFAULT_DEBUGMASK;
}

bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message)
{
    bool sendAck = false;
    updateReportedValues = false;
    JSON_Value *root_value = json_parse_string(message);

#ifdef LOGGING
    int msgLength = strlen(message);
    for (int i = 0; i < msgLength; i += 100) {
        char szBuffer[101];
        snprintf(szBuffer, min(100, msgLength - i), message+i);
        DEBUGMSG(ZONE_RAWDATA, "%s", szBuffer);
    }
#endif

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        DEBUGMSG(ZONE_ERROR, "%s(%d) - parse %s failed", FUNC_NAME, __LINE__, message);
        return sendAck;
    }

    sendAck = true;
    JSON_Object *root_object = json_value_get_object(root_value);

    if (json_object_has_value(root_object, "desired")) {
        updateReportedValues = true;
        for (int iValues = 0; iValues < iExpectedValues; iValues++) {
            char szDesiredValue[80];    // Potential Risk of overflowing the array
            sprintf(szDesiredValue, "desired.%s.value", deviceTwinEntries[iValues].pszName);
            if (json_object_dothas_value(root_object, szDesiredValue)) {
                switch (deviceTwinEntries[iValues].twinDataType) {
                    case VALUE_INT:
                        desiredTwinValues[iValues].iValue = json_object_dotget_number(root_object, szDesiredValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %d", FUNC_NAME, __LINE__, szDesiredValue, desiredTwinValues[iValues].iValue);
                        break;
                    case VALUE_FLOAT:
                        desiredTwinValues[iValues].fValue = json_object_dotget_number(root_object, szDesiredValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %.1f", FUNC_NAME, __LINE__, szDesiredValue, desiredTwinValues[iValues].fValue);
                        break;
                    case VALUE_BOOL:
                        desiredTwinValues[iValues].bValue = json_object_dotget_boolean(root_object, szDesiredValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %d", FUNC_NAME, __LINE__, szDesiredValue, desiredTwinValues[iValues].bValue);
                        break;
                    case VALUE_STRING: {
                        char *pszValue = (char *)json_object_dotget_string(root_object, szDesiredValue);
                        int iValueLength = strlen(pszValue);
                        if (desiredTwinValues[iValues].pszValue != NULL) {
                            free(desiredTwinValues[iValues].pszValue);
                            desiredTwinValues[iValues].pszValue = NULL;
                        }
                        desiredTwinValues[iValues].pszValue = (char *)malloc(iValueLength + 1);
                        if (desiredTwinValues[iValues].pszValue == NULL) {
                            DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iValues);
                            sendAck = false;
                            return sendAck;
                        }
                        snprintf(desiredTwinValues[iValues].pszValue, iValueLength + 1, "%s", pszValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = desiredTwinValues[%d]: %s", FUNC_NAME, __LINE__, szDesiredValue, iValues, desiredTwinValues[iValues].pszValue);
                        break;
                    }
                    default:
                        DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for desiredTwinValues[%d] = %d", FUNC_NAME, __LINE__, iValues, (int)deviceTwinEntries[iValues].twinDataType);
                        sendAck = false;
                        return sendAck;
                }
            }
            char szReportedValue[80];    // Potential Risk of overflowing the array
            sprintf(szReportedValue, "reported.%s", deviceTwinEntries[iValues].pszName);
            if (json_object_dothas_value(root_object, szReportedValue)) {
                hasReportedValues = true;
                switch (deviceTwinEntries[iValues].twinDataType) {
                    case VALUE_INT:
                        reportedTwinValues[iValues].iValue = json_object_dotget_number(root_object, szReportedValue);
                        if (reportedTwinValues[iValues].iValue != desiredTwinValues[iValues].iValue) {
                            reportedTwinValues[iValues].iValue = desiredTwinValues[iValues].iValue;
                        }
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %d", FUNC_NAME, __LINE__, szReportedValue, iValues, reportedTwinValues[iValues].iValue);
                        break;
                    case VALUE_FLOAT:
                        reportedTwinValues[iValues].fValue = json_object_dotget_number(root_object, szReportedValue);
                        if (reportedTwinValues[iValues].fValue != desiredTwinValues[iValues].fValue) {
                            updateReportedValues = true;
                            reportedTwinValues[iValues].fValue = desiredTwinValues[iValues].fValue;
                        }
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %.1f", FUNC_NAME, __LINE__, szReportedValue, iValues, reportedTwinValues[iValues].fValue);
                        break;
                    case VALUE_BOOL:
                        reportedTwinValues[iValues].bValue = json_object_dotget_boolean(root_object, szReportedValue);
                        if (reportedTwinValues[iValues].bValue != desiredTwinValues[iValues].bValue) {
                            updateReportedValues = true;
                            reportedTwinValues[iValues].bValue = desiredTwinValues[iValues].bValue;
                        }
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %d", FUNC_NAME, __LINE__, szReportedValue, iValues, reportedTwinValues[iValues].bValue);
                        break;
                    case VALUE_STRING: {
                        char *pszValue = (char *)json_object_dotget_string(root_object, szReportedValue);
                        int iValueLength = strlen(pszValue);
                        if (reportedTwinValues[iValues].pszValue != NULL) {
                            free(reportedTwinValues[iValues].pszValue);
                            reportedTwinValues[iValues].pszValue = NULL;
                        }
                        if (desiredTwinValues[iValues].pszValue != NULL) {
                            if (strcmp(pszValue, desiredTwinValues[iValues].pszValue) != 0)
                            {
                                pszValue = desiredTwinValues[iValues].pszValue;
                                iValueLength = strlen(pszValue);
                                updateReportedValues = true;
                            }
                        }
                        reportedTwinValues[iValues].pszValue = (char *)malloc(iValueLength + 1);
                        if (reportedTwinValues[iValues].pszValue == NULL) {
                            DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iValues);
                            sendAck = false;
                            return sendAck;
                        }
                        snprintf(reportedTwinValues[iValues].pszValue, iValueLength + 1, "%s", pszValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %s", FUNC_NAME, __LINE__, szReportedValue, iValues, reportedTwinValues[iValues].pszValue);
                        break;
                    }
                    default:
                        DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for reportedTwinValues[%d] = %d", FUNC_NAME, __LINE__, iValues, (int)deviceTwinEntries[iValues].twinDataType);
                        sendAck = false;
                        return sendAck;
                }
            }
        }

        if (! hasReportedValues) {
            // New Device: No reported properties yet in Device Twin. Let's set reported to desired and send them to the IoT Central Application.
            for (int iValues = 0; iValues < iExpectedValues; iValues++) {
                switch (deviceTwinEntries[iValues].twinDataType) {
                    case VALUE_INT:
                        reportedTwinValues[iValues].iValue = desiredTwinValues[iValues].iValue;
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> reportedTwinValues[%d]: %d", FUNC_NAME, __LINE__, iValues, reportedTwinValues[iValues].iValue);
                        break;
                    case VALUE_FLOAT:
                        reportedTwinValues[iValues].fValue = desiredTwinValues[iValues].fValue;
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> reportedTwinValues[%d]: %.1f", FUNC_NAME, __LINE__, iValues, reportedTwinValues[iValues].fValue);
                        break;
                    case VALUE_BOOL:
                        reportedTwinValues[iValues].bValue = desiredTwinValues[iValues].bValue;
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> reportedTwinValues[%d]: %d", FUNC_NAME, __LINE__, iValues, reportedTwinValues[iValues].bValue);
                        break;
                    case VALUE_STRING:
                        break;
                    default:
                        DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for reportedTwinValues[%d] = %d", FUNC_NAME, __LINE__, iValues, (int)deviceTwinEntries[iValues].twinDataType);
                        sendAck = false;
                        return sendAck;
                }
            }
        }
        initialDeviceTwinReportReceived = true;
    } else {
        for (int iNewDesiredValue = 0; iNewDesiredValue < iExpectedValues; iNewDesiredValue++) {
            char szNewDesiredValue[80];    // Potential Risk of overflowing the array
            updateReportedValues = true;
            sprintf(szNewDesiredValue, "%s.value", deviceTwinEntries[iNewDesiredValue].pszName);
            if (json_object_dothas_value(root_object, szNewDesiredValue)) {
                switch (deviceTwinEntries[iNewDesiredValue].twinDataType) {
                    case VALUE_INT:
                        desiredTwinValues[iNewDesiredValue].iValue = json_object_dotget_number(root_object, szNewDesiredValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %d", FUNC_NAME, __LINE__, szNewDesiredValue, desiredTwinValues[iNewDesiredValue].iValue);
                        reportedTwinValues[iNewDesiredValue].iValue = desiredTwinValues[iNewDesiredValue].iValue;
                        if (iNewDesiredValue == IDX_DEBUGMASK) {
#ifdef LOGGING
                            dpCurSettings.ulZoneMask = desiredTwinValues[iNewDesiredValue].iValue;
#endif
                        }
                        break;
                    case VALUE_FLOAT:
                        desiredTwinValues[iNewDesiredValue].fValue = json_object_dotget_number(root_object, szNewDesiredValue);
                        reportedTwinValues[iNewDesiredValue].iValue = desiredTwinValues[iNewDesiredValue].iValue;
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %.1f", FUNC_NAME, __LINE__, szNewDesiredValue, desiredTwinValues[iNewDesiredValue].fValue);
                        break;
                    case VALUE_BOOL:
                        desiredTwinValues[iNewDesiredValue].bValue = json_object_dotget_boolean(root_object, szNewDesiredValue);
                        reportedTwinValues[iNewDesiredValue].iValue = desiredTwinValues[iNewDesiredValue].iValue;
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %d", FUNC_NAME, __LINE__, szNewDesiredValue, desiredTwinValues[iNewDesiredValue].bValue);
                        break;
                    case VALUE_STRING: {
                        char *pszValue = (char *)json_object_dotget_string(root_object, szNewDesiredValue);
                        int iValueLength = strlen(pszValue);

                        if (desiredTwinValues[iNewDesiredValue].pszValue != NULL) {
                            free(desiredTwinValues[iNewDesiredValue].pszValue);
                            desiredTwinValues[iNewDesiredValue].pszValue = NULL;
                        }
                        desiredTwinValues[iNewDesiredValue].pszValue = (char *)malloc(iValueLength + 1);
                        if (desiredTwinValues[iNewDesiredValue].pszValue == NULL) {
                            DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iNewDesiredValue);
                            sendAck = false;
                            return sendAck;
                        }
                        snprintf(desiredTwinValues[iNewDesiredValue].pszValue, iValueLength + 1, "%s", pszValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = desiredTwinValues[%d]: %s", FUNC_NAME, __LINE__, szNewDesiredValue, iNewDesiredValue, desiredTwinValues[iNewDesiredValue].pszValue);

                        if (reportedTwinValues[iNewDesiredValue].pszValue != NULL) {
                            free(reportedTwinValues[iNewDesiredValue].pszValue);
                            reportedTwinValues[iNewDesiredValue].pszValue = NULL;
                        }
                        reportedTwinValues[iNewDesiredValue].pszValue = (char *)malloc(iValueLength + 1);
                        if (reportedTwinValues[iNewDesiredValue].pszValue == NULL) {
                            DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iNewDesiredValue);
                            sendAck = false;
                            return sendAck;
                        }
                        snprintf(reportedTwinValues[iNewDesiredValue].pszValue, iValueLength + 1, "%s", pszValue);
                        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %s", FUNC_NAME, __LINE__, szNewDesiredValue, iNewDesiredValue, reportedTwinValues[iNewDesiredValue].pszValue);
                        break;
                    }
                    default:
                        DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for desiredTwinValues[%d] = %d", FUNC_NAME, __LINE__, iNewDesiredValue, (int)deviceTwinEntries[iNewDesiredValue].twinDataType);
                        sendAck = false;
                        return sendAck;
                }
            }
        }
    }
    json_value_free(root_value);
    isInitialized = true;
    return sendAck;
}

bool CreateTelemetryMessage(char *payload, bool forceCreate)
{
    bool bReadHumidity = reportedTwinValues[IDX_READHUMIDITY].bValue;
    bool bReadPressure = reportedTwinValues[IDX_READPRESSURE].bValue;
    float t = ReadTemperature() + reportedTwinValues[IDX_TEMPERATURECORRECTION].fValue;
    float h = ReadHumidity() + reportedTwinValues[IDX_HUMIDITYCORRECTION].fValue;
    float p = ReadPressure() + reportedTwinValues[IDX_PRESSURECORRECTION].fValue;
    float deltaT = abs(telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue - t);
    float deltaH = abs(telemetryValues[IDX_HUMIDITY_TELEMETRY].fValue - h);
    float deltaP = abs(telemetryValues[IDX_PRESSURE_TELEMETRY].fValue - p);
    bool temperatureAlert = false;

    DEBUGMSG(ZONE_INIT, "--> %s(forceCreate = %d)", FUNC_NAME, __LINE__, forceCreate);
    DEBUGMSG(ZONE_SENSORDATA, "        deltaT = %.2f, deltaH = %.2f, deltaP = %.2f", deltaT, deltaH, deltaP);
    DEBUGMSG(ZONE_SENSORDATA, "        reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue = %.1f, reportedTwinValues[IDX_MAXDELTABETWEENMEASUREMENTS].fValue = %.1f", reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue, reportedTwinValues[IDX_MAXDELTABETWEENMEASUREMENTS].fValue);
    DEBUGMSG(ZONE_SENSORDATA, "        reportedTwinValues[IDX_HUMIDITYACCURACY].fValue = %.1f", reportedTwinValues[IDX_HUMIDITYACCURACY].fValue);
    DEBUGMSG(ZONE_SENSORDATA, "        reportedTwinValues[IDX_PRESSUREACCURACY].fValue = %.1f", reportedTwinValues[IDX_PRESSUREACCURACY].fValue);

    // Correct for sensor jitter
    bool temperatureChanged = (deltaT > reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue) && (deltaT < reportedTwinValues[IDX_MAXDELTABETWEENMEASUREMENTS].fValue);
    bool humidityChanged    = (deltaH > reportedTwinValues[IDX_HUMIDITYACCURACY].fValue)    && (deltaH < reportedTwinValues[IDX_MAXDELTABETWEENMEASUREMENTS].fValue);
    bool pressureChanged    = (deltaP > reportedTwinValues[IDX_PRESSUREACCURACY].fValue)    && (deltaP < reportedTwinValues[IDX_MAXDELTABETWEENMEASUREMENTS].fValue);

    if (forceCreate) {
        temperatureChanged = humidityChanged = pressureChanged = true;
    }

    DEBUGMSG(ZONE_SENSORDATA, "        temperatureChanged = %d, humidityChanged = %d, pressureChanged = %d", temperatureChanged, humidityChanged, pressureChanged);
    DEBUGMSG(ZONE_SENSORDATA, "        temperature = %.1f, humidity = %.1f, pressure = %.1f", telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue, telemetryValues[IDX_HUMIDITY_TELEMETRY].fValue, telemetryValues[IDX_PRESSURE_TELEMETRY].fValue);
    DEBUGMSG(ZONE_SENSORDATA, "        t = %.1f, h = %.1f, p = %.1f", t, h, p);

    if (temperatureChanged || humidityChanged || pressureChanged) {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        char *serialized_string = NULL;

        json_object_set_string(root_object, JSON_DEVICEID, DEVICE_ID);

        if (temperatureChanged) {
            telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue = t;
            json_object_set_number(root_object, telemetryEntries[IDX_TEMPERATURE_TELEMETRY].pszName, Round(telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue));
        }

        if(telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue > DEFAULT_TEMPERATURE_ALERT) {
            temperatureAlert = true;
        }

        if (humidityChanged) {
            telemetryValues[IDX_HUMIDITY_TELEMETRY].fValue = h;
            if (bReadHumidity) {
                json_object_set_number(root_object, telemetryEntries[IDX_HUMIDITY_TELEMETRY].pszName, Round(telemetryValues[IDX_HUMIDITY_TELEMETRY].fValue));
            }
        }

        if (pressureChanged) {
            telemetryValues[IDX_PRESSURE_TELEMETRY].fValue = p;
            if (bReadPressure) {
                json_object_set_number(root_object, telemetryEntries[IDX_PRESSURE_TELEMETRY].pszName, Round(telemetryValues[IDX_PRESSURE_TELEMETRY].fValue));
            }
        }

        ShowTelemetryData(telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue,
                          telemetryValues[IDX_HUMIDITY_TELEMETRY].fValue,
                          telemetryValues[IDX_PRESSURE_TELEMETRY].fValue,
                          reportedTwinValues[IDX_READHUMIDITY].bValue,
                          reportedTwinValues[IDX_READPRESSURE].bValue);

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
    DEBUGMSG(ZONE_INIT, "--> %s()", FUNC_NAME);
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

bool SendDeviceInfo()
{
    char reportedProperties[1024];
    updateReportedValues = false;

    DEBUGMSG(ZONE_INIT, "--> %s()", FUNC_NAME);
    reportedProperties[0] = '{';
    reportedProperties[1] = '\0';

    for (int iValues = 0; iValues < iExpectedValues; iValues++) {
        int idx = strlen(reportedProperties);
        if (iValues == IDX_CURRENTFWVERSION) {
            sprintf(&reportedProperties[idx], "\"Firmware\":");
        } else {
            sprintf(&reportedProperties[idx], "\"%s\":", deviceTwinEntries[iValues].pszName);
        }

        idx = strlen(reportedProperties);
        switch (deviceTwinEntries[iValues].twinDataType) {
            case VALUE_INT:
                sprintf(&reportedProperties[idx], "%d,", reportedTwinValues[iValues].iValue);
                break;
            case VALUE_FLOAT:
                sprintf(&reportedProperties[idx], "%.1f,", reportedTwinValues[iValues].fValue);
                break;
            case VALUE_BOOL:
                sprintf(&reportedProperties[idx], "%s,", reportedTwinValues[iValues].bValue ? "true" : "false");
                break;
            case VALUE_STRING:
                sprintf(&reportedProperties[idx], "\"%s\",", reportedTwinValues[iValues].pszValue);
                break;
            default:
                return false;
        }
    }

    int idx = strlen(reportedProperties);
    sprintf(&reportedProperties[idx-1], "%s", "}");

#ifdef LOGGING
    int msgLength = strlen(reportedProperties);
    for (int i = 0; i < msgLength; i += 100) {
        char szBuffer[101];
        snprintf(szBuffer, min(100, msgLength - i), reportedProperties+i);
        DEBUGMSG(ZONE_RAWDATA, "        %s", szBuffer);
    }
#endif

    DEBUGMSG(ZONE_HUBMSG, "    %s() - Sending reportedProperties to IoTHub", FUNC_NAME);
    return DevKitMQTTClient_ReportState(reportedProperties);
}

bool InitialDeviceTwinDesiredReceived()
{
    DEBUGMSG(ZONE_VERBOSE, "%s() - initialDeviceTwinReportReceived = %d", FUNC_NAME, initialDeviceTwinReportReceived);
    return initialDeviceTwinReportReceived;
}

int MeasurementInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "%s() - isInitialized = %d, interval = %d", FUNC_NAME, isInitialized, reportedTwinValues[IDX_MEASUREINTERVAL].iValue * 1000);
    return isInitialized ? reportedTwinValues[IDX_MEASUREINTERVAL].iValue * 1000 : -1;
}

int SendInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "%s() - isInitialized = %d, interval = %d", FUNC_NAME, isInitialized, reportedTwinValues[IDX_SENDINTERVAL].iValue * 1000);
    return isInitialized ? reportedTwinValues[IDX_SENDINTERVAL].iValue * 1000 : -1;
}

int SleepInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "%s() - isInitialized = %d, interval = %d", FUNC_NAME, isInitialized, reportedTwinValues[IDX_SLEEPTIME].iValue);
    return isInitialized ? reportedTwinValues[IDX_SLEEPTIME].iValue : -1;
}

int MotionInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "%s() - isInitialized = %d, interval = %d", FUNC_NAME, isInitialized, reportedTwinValues[IDX_MOTIONDETECTIONINTERVAL].iValue * 1000);
    return isInitialized ? reportedTwinValues[IDX_MOTIONDETECTIONINTERVAL].iValue * 1000 : -1;
}

int WarmingUpTime()
{
    DEBUGMSG(ZONE_VERBOSE, "%s() - isInitialized = %d, interval = %d", FUNC_NAME, isInitialized,  reportedTwinValues[IDX_WARMINGUPTIME].iValue * 60 * 1000);
    return isInitialized ? reportedTwinValues[IDX_WARMINGUPTIME].iValue * 60 * 1000 : -1;
}

int MotionSensitivity()
{
    return isInitialized ? reportedTwinValues[IDX_MOTIONSENSITIVITY].iValue : -1;
}

bool MotionDetectionEnabled()
{
    return isInitialized ? reportedTwinValues[IDX_ENABLEMOTIONDETECTION].bValue : false;
}

bool UpdateReportedValues()
{
    return updateReportedValues;
}