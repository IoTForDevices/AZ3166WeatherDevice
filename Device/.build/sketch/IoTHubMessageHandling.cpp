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
    { VALUE_INT,    "actualDebugMask" }
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
DEVICE_DATA telemetryValues[iTelemetryValues];

/* Helper Functions to parse Twin Messages */

/******************************************************************************************************************************************************
 * Set the reported device twin integer value equal to the desired integer value when they are different or when a reported value does not yet exist
 * 
 * int iValues = Index in the array of reported / desired twin values
 * char *pszReportedValue = json object containing the name of the reported twin value or NULL if no reported value detected.
 * JSON_OBJECT *root_object = pointer to the Device Twin json payload, coming from IoT Hub
 *****************************************************************************************************************************************************/
static void SetReportedIntValue(int iValues, char *pszReportedValue, JSON_Object *root_object)
{
    bool updateValue = true;

    if (pszReportedValue != NULL) {
        reportedTwinValues[iValues].iValue = json_object_dotget_number(root_object, pszReportedValue);
        if (reportedTwinValues[iValues].iValue == desiredTwinValues[iValues].iValue) {
            updateValue = false;
        }
    }
    
    if (updateValue) {
        updateReportedValues = true;
        reportedTwinValues[iValues].iValue = desiredTwinValues[iValues].iValue;
    }

    DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %d", FUNC_NAME, __LINE__, pszReportedValue != NULL ? pszReportedValue : "", iValues, reportedTwinValues[iValues].iValue);
}

/******************************************************************************************************************************************************
 * Set the reported device twin floating point value equal to the desired floating point value when they are different 
 * or when a reported value does not yet exist
 * 
 * int iValues = Index in the array of reported / desired twin values
 * char *pszReportedValue = json object containing the name of the reported twin value or NULL if no reported value detected.
 * JSON_OBJECT *root_object = pointer to the Device Twin json payload, coming from IoT Hub
 *****************************************************************************************************************************************************/
static void SetReportedFloatValue(int iValues, char *pszReportedValue, JSON_Object *root_object)
{
    bool updateValue = true;

    if (pszReportedValue != NULL) {
        reportedTwinValues[iValues].fValue = json_object_dotget_number(root_object, pszReportedValue);
        if (reportedTwinValues[iValues].fValue == desiredTwinValues[iValues].fValue) {
            updateValue = false;
        }
    }
    
    if (updateValue) {
        updateReportedValues = true;
        reportedTwinValues[iValues].fValue = desiredTwinValues[iValues].fValue;
    }

    DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %.1f", FUNC_NAME, __LINE__, pszReportedValue != NULL ? pszReportedValue : "", iValues, reportedTwinValues[iValues].fValue);
}

/******************************************************************************************************************************************************
 * Set the reported device twin boolean value equal to the desired boolean value when they are different 
 * or when a reported value does not yet exist
 * 
 * int iValues = Index in the array of reported / desired twin values
 * char *pszReportedValue = json object containing the name of the reported twin value or NULL if no reported value detected.
 * JSON_OBJECT *root_object = pointer to the Device Twin json payload, coming from IoT Hub
 *****************************************************************************************************************************************************/
static void SetReportedBoolValue(int iValues, char *pszReportedValue, JSON_Object *root_object)
{
    bool updateValue = true;

    if (pszReportedValue != NULL) {
        reportedTwinValues[iValues].bValue = json_object_dotget_number(root_object, pszReportedValue);
        if (reportedTwinValues[iValues].bValue == desiredTwinValues[iValues].bValue) {
            updateValue = false;
        }
    }
    
    if (updateValue) {
        updateReportedValues = true;
        reportedTwinValues[iValues].bValue = desiredTwinValues[iValues].bValue;
    }

    DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %d", FUNC_NAME, __LINE__, pszReportedValue != NULL ? pszReportedValue : "", iValues, reportedTwinValues[iValues].bValue);
}

/******************************************************************************************************************************************************
 * Set the reported device twin string value equal to the desired string value when they are different 
 * or when a reported value does not yet exist
 * 
 * int iValues = Index in the array of reported / desired twin values
 * char *pszReportedValue = json object containing the name of the reported twin value or NULL if no reported value detected.
 * JSON_OBJECT *root_object = pointer to the Device Twin json payload, coming from IoT Hub
 * 
 * Return value = bool - True if value properly set, otherwise false (mostly due to allocation errors)
 *****************************************************************************************************************************************************/
static bool SetReportedStringValue(int iValues, char *pszReportedValue, JSON_Object *root_object)
{
    bool updateValue = true;
    bool sendAck = true;
    char *pszValue = NULL;

    if (pszReportedValue != NULL) {
        pszValue = (char *)json_object_dotget_string(root_object, pszReportedValue);

        if (desiredTwinValues[iValues].pszValue != NULL) {
            if (strcmp(pszValue, desiredTwinValues[iValues].pszValue) == 0)
            {
                updateValue = false;
            }
        }
    }

    if (updateValue) {
        if (desiredTwinValues[iValues].pszValue != NULL) {
            // If we have a desired Twin Value, assign it to the reported Twin Value as well and feed back to IoTHub
            char *pszDesiredValue = desiredTwinValues[iValues].pszValue;            
            int iValueLength = strlen(pszDesiredValue);
            
            if (reportedTwinValues[iValues].pszValue != NULL) {
                free(reportedTwinValues[iValues].pszValue);
                reportedTwinValues[iValues].pszValue = NULL;
            }

            reportedTwinValues[iValues].pszValue = (char *)malloc(iValueLength + 1);
            if (reportedTwinValues[iValues].pszValue == NULL) {
                DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iValues);
                sendAck = false;
                return sendAck;
            }
            sprintf(reportedTwinValues[iValues].pszValue, "%s", pszDesiredValue);
            updateReportedValues = true;
        } else {
            // If we don't have a desired Twin Value, just assign the reported value to the string received from IoT Hub
            if (reportedTwinValues[iValues].pszValue != NULL) {
                free(reportedTwinValues[iValues].pszValue);
                reportedTwinValues[iValues].pszValue = NULL;
            }

            if (pszValue == NULL) {
                DEBUGMSG(ZONE_ERROR, "%s(%d) - No reported value found for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iValues);
                sendAck = false;
                return sendAck;
            }
            reportedTwinValues[iValues].pszValue = (char *)malloc(strlen(pszValue) + 1);
            if (reportedTwinValues[iValues].pszValue == NULL) {
                DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iValues);
                sendAck = false;
                return sendAck;
            }
            sprintf(reportedTwinValues[iValues].pszValue, "%s", pszValue);
        }
    }

    if (pszReportedValue != NULL) {
        DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = reportedTwinValues[%d]: %s", FUNC_NAME, __LINE__, pszReportedValue, iValues, reportedTwinValues[iValues].pszValue);
    }

    return sendAck;
}

/******************************************************************************************************************************************************
 * Set the reported Twin Value to the value that was reported by IoT Hub 
 * 
 * int iTwinEntry = Index in the array of desired twin values for which the desired value has to be set.
 * JSON_Object *root_object = pointer to the Device Twin's json payload, coming from IoT Hub
 * char *pszReportedTwinValue = json object containing the name of the reported twin value.
 * 
 * Return value = bool - True if value properly set, otherwise false (mostly due to allocation errors)
 *****************************************************************************************************************************************************/
static bool SetReportedTwinValue(int iTwinEntry, JSON_Object *root_object, char *pszReportedTwinValue)
{
    hasReportedValues = true;
    switch (deviceTwinEntries[iTwinEntry].twinDataType) {
        case VALUE_INT:
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s", FUNC_NAME, __LINE__, pszReportedTwinValue != NULL ? pszReportedTwinValue : "NULL");
            SetReportedIntValue(iTwinEntry, pszReportedTwinValue, root_object);
#ifdef LOGGING
            if (iTwinEntry == IDX_DEBUGMASK) {
                reportedTwinValues[iTwinEntry].iValue = dpCurSettings.ulZoneMask;
            }
#endif
            break;
        case VALUE_FLOAT:
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s", FUNC_NAME, __LINE__, pszReportedTwinValue != NULL ? pszReportedTwinValue : "NULL");
            SetReportedFloatValue(iTwinEntry, pszReportedTwinValue, root_object);
            break;
        case VALUE_BOOL:
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s", FUNC_NAME, __LINE__, pszReportedTwinValue != NULL ? pszReportedTwinValue : "NULL");
            SetReportedBoolValue(iTwinEntry, pszReportedTwinValue, root_object);
            break;
        case VALUE_STRING: {
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s", FUNC_NAME, __LINE__, pszReportedTwinValue != NULL ? pszReportedTwinValue : "NULL");
            if (SetReportedStringValue(iTwinEntry, pszReportedTwinValue, root_object) == false) {
                return false;
            }
            break;
        }
        default:
            DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for reportedTwinValues[%d] = %d", FUNC_NAME, __LINE__, iTwinEntry, (int)deviceTwinEntries[iTwinEntry].twinDataType);
            return false;
    }
    return true;
}

/******************************************************************************************************************************************************
 * Set the desired Twin Value to the value that was reported by IoT Hub 
 * 
 * int iTwinEntry = Index in the array of desired twin values for which the desired value has to be set.
 * JSON_Object *root_object = pointer to the Device Twin's json payload, coming from IoT Hub
 * char *pszDesiredTwinValue = json object containing the name of the desired twin value.
 * bool bNewDesiredValue = TRUE if there is a new desired value that also be set as reported value, else false.
 * 
 * Return value = bool - True if value properly set, otherwise false (mostly due to allocation errors)
 *****************************************************************************************************************************************************/
static bool SetDesiredTwinValue(int iTwinEntry, JSON_Object *root_object, char *pszDesiredTwinValue, bool bNewDesiredValue)
{
    switch (deviceTwinEntries[iTwinEntry].twinDataType) {
        case VALUE_INT:
            desiredTwinValues[iTwinEntry].iValue = json_object_dotget_number(root_object, pszDesiredTwinValue);
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %d", FUNC_NAME, __LINE__, pszDesiredTwinValue, desiredTwinValues[iTwinEntry].iValue);
            break;
        case VALUE_FLOAT:
            desiredTwinValues[iTwinEntry].fValue = json_object_dotget_number(root_object, pszDesiredTwinValue);
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %.1f", FUNC_NAME, __LINE__, pszDesiredTwinValue, desiredTwinValues[iTwinEntry].fValue);
            break;
        case VALUE_BOOL:
            desiredTwinValues[iTwinEntry].bValue = json_object_dotget_boolean(root_object, pszDesiredTwinValue);
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %d", FUNC_NAME, __LINE__, pszDesiredTwinValue, desiredTwinValues[iTwinEntry].bValue);
            break;
        case VALUE_STRING: {
            char *pszValue = (char *)json_object_dotget_string(root_object, pszDesiredTwinValue);
            int iValueLength = strlen(pszValue);
            if (desiredTwinValues[iTwinEntry].pszValue != NULL) {
                free(desiredTwinValues[iTwinEntry].pszValue);
                desiredTwinValues[iTwinEntry].pszValue = NULL;
            }
            desiredTwinValues[iTwinEntry].pszValue = (char *)malloc(iValueLength + 1);
            if (desiredTwinValues[iTwinEntry].pszValue == NULL) {
                DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, iTwinEntry);
                return false;
            }
            sprintf(desiredTwinValues[iTwinEntry].pszValue, "%s", pszValue);
            DEBUGMSG(ZONE_TWINPARSING, "%s(%d) >> %s = %s", FUNC_NAME, __LINE__, pszDesiredTwinValue, desiredTwinValues[iTwinEntry].pszValue);
            break;
        }
        default:
            DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for desiredTwinValues[%d] = %d", FUNC_NAME, __LINE__, iTwinEntry, (int)deviceTwinEntries[iTwinEntry].twinDataType);
            return false;
    }

    return bNewDesiredValue ? SetReportedTwinValue(iTwinEntry, NULL, NULL) : true;
}

/* End of Helper Functions to parse Twin Messages */

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

    reportedTwinValues[IDX_DEVICEMODEL].pszValue = (char *)malloc(strlen(DEVICE_ID) + 1);
    if (reportedTwinValues[IDX_DEVICEMODEL].pszValue == NULL) {
        DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, IDX_DEVICEMODEL);
    } else {
        snprintf(reportedTwinValues[IDX_DEVICEMODEL].pszValue, strlen(DEVICE_ID), "%s", DEVICE_ID);
    }

    reportedTwinValues[IDX_CURRENTFWVERSION].pszValue = (char *)malloc(strlen(DEVICE_FIRMWARE_VERSION) + 1);
    if (reportedTwinValues[IDX_CURRENTFWVERSION].pszValue == NULL) {
        DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, IDX_CURRENTFWVERSION);
    } else {
        snprintf(reportedTwinValues[IDX_CURRENTFWVERSION].pszValue, strlen(DEVICE_FIRMWARE_VERSION), "%s", DEVICE_FIRMWARE_VERSION);
    }

    reportedTwinValues[IDX_DEVICELOCATION].pszValue = (char *)malloc(strlen(DEVICE_LOCATION) + 1);
    if (reportedTwinValues[IDX_DEVICELOCATION].pszValue == NULL) {
        DEBUGMSG(ZONE_ERROR, "%s(%d) - Memory allocation failed for reportedTwinValues[%d]", FUNC_NAME, __LINE__, IDX_DEVICELOCATION);
    } else {
        snprintf(reportedTwinValues[IDX_DEVICELOCATION].pszValue, strlen(DEVICE_LOCATION), "%s", DEVICE_LOCATION);
    }
}



bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, int msgLength)
{
    updateReportedValues = false;
    JSON_Value *root_value = json_parse_string(message);

#ifdef LOGGING
    for (int i = 0; i < msgLength; i += 200) {
        char szBuffer[201];
        snprintf(szBuffer, min(201, msgLength - i + 1), message+i);
        DEBUGMSG(ZONE_RAWDATA, "%s", szBuffer);
    }
#endif

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        DEBUGMSG(ZONE_ERROR, "%s(%d) - parse %s failed", FUNC_NAME, __LINE__, message);
        return false;
    }

    JSON_Object *root_object = json_value_get_object(root_value);

    if (json_object_has_value(root_object, "desired")) {
        updateReportedValues = true;
        for (int iValues = 0; iValues < iExpectedValues; iValues++) {
            char szDesiredValue[80];    // Potential Risk of overflowing the array
            sprintf(szDesiredValue, "desired.%s.value", deviceTwinEntries[iValues].pszName);
            if (json_object_dothas_value(root_object, szDesiredValue)) {
                if (! SetDesiredTwinValue(iValues, root_object, szDesiredValue, false)) {
                    return false;
                }
            }

            char szReportedValue[80];    // Potential Risk of overflowing the array
            sprintf(szReportedValue, "reported.%s", deviceTwinEntries[iValues].pszName);
            if (json_object_dothas_value(root_object, szReportedValue)) {
                if (! SetReportedTwinValue(iValues, root_object, szReportedValue)) {
                    return false;
                }
            } else {    // We have a new desired value that does not yet have a reported value, so assign the desired value to the reported value immediately.
                updateReportedValues = true;
                switch (deviceTwinEntries[iValues].twinDataType) {
                    case VALUE_INT:
                        SetReportedIntValue(iValues, NULL, NULL);
                        break;
                    case VALUE_FLOAT:
                        SetReportedFloatValue(iValues, NULL, NULL);
                        break;
                    case VALUE_BOOL:
                        SetReportedBoolValue(iValues, NULL, NULL);
                        break;
                    case VALUE_STRING:
                        if (! SetReportedStringValue(iValues, NULL, NULL)) {
                            return false;
                        }
                        break;
                    default:
                        DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for reportedTwinValues[%d] = %d", FUNC_NAME, __LINE__, iValues, (int)deviceTwinEntries[iValues].twinDataType);
                        return false;
                }
            }
        }

        if (! hasReportedValues) {
            // New Device: No reported properties yet in Device Twin. Let's set reported to desired and send them to the IoT Central Application.
            for (int iValues = 0; iValues < iExpectedValues; iValues++) {
                switch (deviceTwinEntries[iValues].twinDataType) {
                    case VALUE_INT:
                        SetReportedIntValue(iValues, NULL, NULL);
                        break;
                    case VALUE_FLOAT:
                        SetReportedFloatValue(iValues, NULL, NULL);
                        break;
                    case VALUE_BOOL:
                        SetReportedBoolValue(iValues, NULL, NULL);
                        break;
                    case VALUE_STRING:
                        if (! SetReportedStringValue(iValues, NULL, NULL)) {
                            return false;
                        }
                        break;
                    default:
                        DEBUGMSG(ZONE_ERROR, "%s(%d) - Undefined Twin Datatype for reportedTwinValues[%d] = %d", FUNC_NAME, __LINE__, iValues, (int)deviceTwinEntries[iValues].twinDataType);
                        return false;
                }
            }
        }
        initialDeviceTwinReportReceived = true;
    } else {
        for (int iNewDesiredValue = 0; iNewDesiredValue < iExpectedValues; iNewDesiredValue++) {
            char szNewDesiredValue[80];    // Potential Risk of overflowing the array
            sprintf(szNewDesiredValue, "%s.value", deviceTwinEntries[iNewDesiredValue].pszName);
            if (json_object_dothas_value(root_object, szNewDesiredValue)) {
                if (! SetDesiredTwinValue(iNewDesiredValue, root_object, szNewDesiredValue, true)) {
                    return false;
                }

            }
        }
    }
    json_value_free(root_value);
    isInitialized = true;
    return true;
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

        if (temperatureChanged) {
            telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue = t;
            json_object_set_number(root_object, telemetryEntries[IDX_TEMPERATURE_TELEMETRY].pszName, Round(telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue));
        }

        if(telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue > reportedTwinValues[IDX_TEMPERATUREALERT].iValue) {
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

    if (eventType == MOTION_EVENT) {
        json_object_set_string(root_object, JSON_EVENT_MOTION, "true");
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
    sprintf(&reportedProperties[idx-1], "%c", '}');

#ifdef LOGGING
    for (int i = 0; i < idx; i += 200) {
        char szBuffer[201];
        snprintf(szBuffer, min(201, idx - i + 1), reportedProperties+i);
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

char *CurrentFWVersion()
{
    return isInitialized ? reportedTwinValues[IDX_CURRENTFWVERSION].pszValue : NULL;
}