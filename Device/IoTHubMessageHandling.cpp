#line 1 "IoTHubMessageHandling.cpp"
#include "Arduino.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "parson.h"
#include "config.h"

#include "ReadSensorData.h"
#include "AZ3166Leds.h"
#include "IoTHubMessageHandling.h"
#include "Utils.h"
#include "DebugZones.h"

static bool initialDeviceTwinReportReceived = false;
static bool hasReportedValues = false;
static bool isInitialized = false;
static bool updateReportedValues = false;

DEVICE_ENTRIES deviceTwinEntries[] = {
    { VALUE_INT,    "measureInterval"           },
    { VALUE_INT,    "sendInterval"              },
    { VALUE_INT,    "warmingUpTime"             },
    { VALUE_INT,    "motionDetectionInterval"   },
    { VALUE_INT,    "sleepTime"                 },
    { VALUE_INT,    "temperatureAlert"          },
    { VALUE_FLOAT,  "temperatureAccuracy"       },
    { VALUE_FLOAT,  "humidityAccuracy"          },
    { VALUE_FLOAT,  "pressureAccuracy"          },
    { VALUE_FLOAT,  "temperatureCorrection"     },
    { VALUE_FLOAT,  "humidityCorrection"        },
    { VALUE_FLOAT,  "pressureCorrection"        },
    { VALUE_BOOL,   "enableHumidityReading"     },
    { VALUE_BOOL,   "enablePressureReading"     },
    { VALUE_BOOL,   "enableMotionDetection"     },
    { VALUE_INT,    "motionSensitivity"         },
    { VALUE_STRING, "firmware.currentFwVersion" },
    { VALUE_STRING, "Model"                     },
    { VALUE_STRING, "Location"                  },
    { VALUE_INT,    "actualDebugMask"           }
};

DEVICE_ENTRIES telemetryEntries[] = {
    { VALUE_FLOAT,  "temperature" },
    { VALUE_FLOAT,  "humidity"    },
    { VALUE_FLOAT,  "pressure"    },
    { VALUE_ULONG,  "upTime"      }
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

    DEBUGMSG_FUNC_IN("(%d, %s, %p)", iValues, ShowString(pszReportedValue), root_object);

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
#ifdef LOGGING
    if (iValues == IDX_DEBUGMASK && updateValue) {
        dpCurSettings.ulZoneMask = reportedTwinValues[iValues].iValue;
    }
#endif
    DEBUGMSG(ZONE_TWINPARSING, "updateValue = %s, reportedTwinValues[%d]: %d", ShowBool(updateValue), iValues, reportedTwinValues[iValues].iValue);
    DEBUGMSG_FUNC_OUT("");
}

/******************************************************************************************************************************************************
 * Set the reported device twin ulong value equal to the desired ulong value when they are different or when a reported value does not yet exist
 * 
 * int iValues = Index in the array of reported / desired twin values
 * char *pszReportedValue = json object containing the name of the reported twin value or NULL if no reported value detected.
 * JSON_OBJECT *root_object = pointer to the Device Twin json payload, coming from IoT Hub
 *****************************************************************************************************************************************************/
static void SetReportedUlongValue(int iValues, char *pszReportedValue, JSON_Object *root_object)
{
    bool updateValue = true;

    DEBUGMSG_FUNC_IN("(%d, %s, %p)", iValues, ShowString(pszReportedValue), root_object);
    if (pszReportedValue != NULL) {
        reportedTwinValues[iValues].ulValue = json_object_dotget_number(root_object, pszReportedValue);
        if (reportedTwinValues[iValues].ulValue == desiredTwinValues[iValues].ulValue) {
            updateValue = false;
        }
    }
    
    if (updateValue) {
        updateReportedValues = true;
        reportedTwinValues[iValues].ulValue = desiredTwinValues[iValues].ulValue;
    }

#ifdef LOGGING
    char szTwinValue[ULONG_64_MAX_DIGITS+1];
    Int64ToString(reportedTwinValues[iValues].ulValue, szTwinValue, ULONG_64_MAX_DIGITS);

    DEBUGMSG(ZONE_TWINPARSING, "updateValue = %s, reportedTwinValues[%d]: %s", ShowBool(updateValue), iValues, szTwinValue);
#endif
    DEBUGMSG_FUNC_OUT("");
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

    DEBUGMSG_FUNC_IN("(%d, %s, %p)", iValues, ShowString(pszReportedValue), root_object);
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

    DEBUGMSG(ZONE_TWINPARSING, "updateValue = %s, reportedTwinValues[%d]: %.1f", ShowBool(updateValue), iValues, reportedTwinValues[iValues].fValue);
    DEBUGMSG_FUNC_OUT("");
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

    DEBUGMSG_FUNC_IN("(%d, %s, %p)", iValues, ShowString(pszReportedValue), root_object);
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

    DEBUGMSG(ZONE_TWINPARSING, "updateValue = %s, reportedTwinValues[%d]: %s", ShowBool(updateValue), iValues, ShowBool(reportedTwinValues[iValues].bValue));
    DEBUGMSG_FUNC_OUT("");
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
    bool bSuccess = true;
    char *pszValue = NULL;

    DEBUGMSG_FUNC_IN("(%d, %s, %p)", iValues, ShowString(pszReportedValue), root_object);
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
                DEBUGMSG(ZONE_ERROR, "Memory allocation failed for reportedTwinValues[%d]", iValues);
                bSuccess = false;
            } else {
                sprintf(reportedTwinValues[iValues].pszValue, "%s", pszDesiredValue);
                updateReportedValues = true;
                DEBUGMSG(ZONE_TWINPARSING, "updateValue = %s, reportedTwinValues[%d]: %s", ShowBool(updateValue), iValues, ShowString(reportedTwinValues[iValues].pszValue));
            }
        } else {
            // If we don't have a desired Twin Value, just assign the reported value to the string received from IoT Hub
            if (reportedTwinValues[iValues].pszValue != NULL) {
                free(reportedTwinValues[iValues].pszValue);
                reportedTwinValues[iValues].pszValue = NULL;
            }

            if (pszValue == NULL) {
                DEBUGMSG(ZONE_ERROR, "No reported value found for reportedTwinValues[%d]", iValues);
                bSuccess = false;
            } else {
                reportedTwinValues[iValues].pszValue = (char *)malloc(strlen(pszValue) + 1);
                if (reportedTwinValues[iValues].pszValue == NULL) {
                    DEBUGMSG(ZONE_ERROR, "Memory allocation failed for reportedTwinValues[%d]", iValues);
                    bSuccess = false;
                } else {
                    sprintf(reportedTwinValues[iValues].pszValue, "%s", pszValue);
                    DEBUGMSG(ZONE_TWINPARSING, "updateValue = %s, reportedTwinValues[%d]: %s", ShowBool(updateValue), iValues, ShowString(reportedTwinValues[iValues].pszValue));
                }
            }
        }
    }

    DEBUGMSG_FUNC_OUT(" = %s", ShowBool(bSuccess));

    return bSuccess;
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
    bool bSuccess = true;

    DEBUGMSG_FUNC_IN("(%d, %p, %s)", iTwinEntry, root_object, ShowString(pszReportedTwinValue));
    hasReportedValues = true;
    switch (deviceTwinEntries[iTwinEntry].twinDataType) {
        case VALUE_INT:
            SetReportedIntValue(iTwinEntry, pszReportedTwinValue, root_object);
#ifdef LOGGING
            if (iTwinEntry == IDX_DEBUGMASK) {
                reportedTwinValues[iTwinEntry].iValue = dpCurSettings.ulZoneMask;
            }
#endif
            break;
        case VALUE_ULONG:
            SetReportedUlongValue(iTwinEntry, pszReportedTwinValue, root_object);
            break;
        case VALUE_FLOAT:
            SetReportedFloatValue(iTwinEntry, pszReportedTwinValue, root_object);
            break;
        case VALUE_BOOL:
            SetReportedBoolValue(iTwinEntry, pszReportedTwinValue, root_object);
            break;
        case VALUE_STRING: {
            bSuccess = SetReportedStringValue(iTwinEntry, pszReportedTwinValue, root_object);
            break;
        }
        default:
            DEBUGMSG(ZONE_ERROR, "Undefined Twin Datatype for reportedTwinValues[%d] = %d", iTwinEntry, (int)deviceTwinEntries[iTwinEntry].twinDataType);
            bSuccess = false;
    }
    DEBUGMSG_FUNC_OUT("= %s", ShowBool(bSuccess));
    return bSuccess;
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
    bool bSuccess = true;

    DEBUGMSG_FUNC_IN("(%d, %p, %s, %s)", iTwinEntry, root_object, ShowString(pszDesiredTwinValue), ShowBool(bNewDesiredValue));
    switch (deviceTwinEntries[iTwinEntry].twinDataType) {
        case VALUE_INT:
            desiredTwinValues[iTwinEntry].iValue = json_object_dotget_number(root_object, pszDesiredTwinValue);
            DEBUGMSG(ZONE_TWINPARSING, "desiredTwinValues[%d].iValue = %d", iTwinEntry, desiredTwinValues[iTwinEntry].iValue);
            break;
        case VALUE_ULONG:
            desiredTwinValues[iTwinEntry].ulValue = json_object_dotget_number(root_object, pszDesiredTwinValue);
#ifdef LOGGING
            char szTwinValue[ULONG_64_MAX_DIGITS+1];
            Int64ToString(desiredTwinValues[iTwinEntry].ulValue, szTwinValue, ULONG_64_MAX_DIGITS);

            DEBUGMSG(ZONE_TWINPARSING, "desiredTwinValues[%d].iValue = %d", iTwinEntry, szTwinValue);
#endif
            break;
        case VALUE_FLOAT:
            desiredTwinValues[iTwinEntry].fValue = json_object_dotget_number(root_object, pszDesiredTwinValue);
            DEBUGMSG(ZONE_TWINPARSING, "desiredTwinValues[%d].fValue = %.1f", iTwinEntry, desiredTwinValues[iTwinEntry].fValue);
            break;
        case VALUE_BOOL:
            desiredTwinValues[iTwinEntry].bValue = json_object_dotget_boolean(root_object, pszDesiredTwinValue);
            DEBUGMSG(ZONE_TWINPARSING, "desiredTwinValues[%d].bValue = %s", iTwinEntry, ShowBool(desiredTwinValues[iTwinEntry].bValue));
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
                DEBUGMSG(ZONE_ERROR, "Memory allocation failed for reportedTwinValues[%d]", iTwinEntry);
                bSuccess = false;
            } else {
                sprintf(desiredTwinValues[iTwinEntry].pszValue, "%s", pszValue);
                DEBUGMSG(ZONE_TWINPARSING, "desiredTwinValues[%d].pszValue = %s", iTwinEntry, ShowString(desiredTwinValues[iTwinEntry].pszValue));
            }
            break;
        }
        default:
            DEBUGMSG(ZONE_ERROR, "Undefined Twin Datatype for desiredTwinValues[%d] = %d", iTwinEntry, (int)deviceTwinEntries[iTwinEntry].twinDataType);
            bSuccess = false;
    }

    if (bNewDesiredValue && bSuccess) {
        bSuccess = SetReportedTwinValue(iTwinEntry, NULL, NULL);
    }

    DEBUGMSG_FUNC_OUT("= %s", ShowBool(bSuccess));
    return bSuccess;
}

/* End of Helper Functions to parse Twin Messages */

/******************************************************************************************************************************************************
 * Set initial reported values to the reportedTwinValues structure in case we connect a new device for the very first time. 
 *****************************************************************************************************************************************************/
void InitializeReportedTwinValues()
{
    DEBUGMSG_FUNC_IN("()");
    reportedTwinValues[IDX_MEASUREINTERVAL].iValue = DEFAULT_MEASURE_INTERVAL; 
    reportedTwinValues[IDX_SENDINTERVAL].iValue = DEFAULT_SEND_INTERVAL; 
    reportedTwinValues[IDX_WARMINGUPTIME].iValue = DEFAULT_WARMING_UP_TIME; 
    reportedTwinValues[IDX_MOTIONDETECTIONINTERVAL].iValue = DEFAULT_MOTION_DETECTION_INTERVAL; 
    reportedTwinValues[IDX_SLEEPTIME].iValue = DEFAULT_SLEEP_INTERVAL; 
    reportedTwinValues[IDX_TEMPERATUREALERT].iValue =  DEFAULT_TEMPERATURE_ALERT;
    reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue = DEFAULT_TEMPERATURE_ACCURACY; 
    reportedTwinValues[IDX_HUMIDITYACCURACY].fValue = DEFAULT_HUMIDITY_ACCURACY; 
    reportedTwinValues[IDX_PRESSUREACCURACY].fValue = DEFAULT_PRESSURE_ACCURACY;
    reportedTwinValues[IDX_TEMPERATURECORRECTION].fValue = DEFAULT_TEMPERATURE_CORRECTION;
    reportedTwinValues[IDX_HUMIDITYCORRECTION].fValue = DEFAULT_HUMIDITY_CORRECTION;
    reportedTwinValues[IDX_PRESSURECORRECTION].fValue = DEFAULT_PRESSURE_CORRECTION;
    reportedTwinValues[IDX_READHUMIDITY].bValue = DEFAULT_READ_HUMIDITY;
    reportedTwinValues[IDX_READPRESSURE].bValue = DEFAULT_READ_PRESSURE;
    reportedTwinValues[IDX_ENABLEMOTIONDETECTION].bValue = DEFAULT_DETECT_MOTION;
    reportedTwinValues[IDX_MOTIONSENSITIVITY].iValue = DEFAULT_MOTION_SENSITIVITY;
#ifdef LOGGING    
    reportedTwinValues[IDX_DEBUGMASK].iValue = dpCurSettings.ulZoneMask;
#else
    reportedTwinValues[IDX_DEBUGMASK].iValue = 0;
#endif
    reportedTwinValues[IDX_DEVICEMODEL].pszValue = (char *)malloc(strlen(DEVICE_ID) + 1);
    if (reportedTwinValues[IDX_DEVICEMODEL].pszValue == NULL) {
        DEBUGMSG(ZONE_ERROR, " [%-25s] %04d-%s, Memory allocation failed for reportedTwinValues[%d]", IDX_DEVICEMODEL);
    } else {
        snprintf(reportedTwinValues[IDX_DEVICEMODEL].pszValue, strlen(DEVICE_ID), "%s", DEVICE_ID);
    }

    reportedTwinValues[IDX_CURRENTFWVERSION].pszValue = (char *)malloc(strlen(DEVICE_FIRMWARE_VERSION) + 1);
    if (reportedTwinValues[IDX_CURRENTFWVERSION].pszValue == NULL) {
        DEBUGMSG(ZONE_ERROR, "Memory allocation failed for reportedTwinValues[%d]", IDX_CURRENTFWVERSION);
    } else {
        snprintf(reportedTwinValues[IDX_CURRENTFWVERSION].pszValue, strlen(DEVICE_FIRMWARE_VERSION), "%s", DEVICE_FIRMWARE_VERSION);
    }

    reportedTwinValues[IDX_DEVICELOCATION].pszValue = (char *)malloc(strlen(DEVICE_LOCATION) + 1);
    if (reportedTwinValues[IDX_DEVICELOCATION].pszValue == NULL) {
        DEBUGMSG(ZONE_ERROR, "Memory allocation failed for reportedTwinValues[%d]", IDX_DEVICELOCATION);
    } else {
        snprintf(reportedTwinValues[IDX_DEVICELOCATION].pszValue, strlen(DEVICE_LOCATION), "%s", DEVICE_LOCATION);
    }
    DEBUGMSG_FUNC_OUT("");
}

/******************************************************************************************************************************************************
 * Set the desired and reported Twin Value to the values that were reported by IoT Hub 
 * 
 * DEVICE_TWIN_UPDATE_STATE updateState = Index in the array of desired twin values for which the desired value has to be set.
 * const char *message = raw data containing the json payload, coming from IoT Hub.
 * int msgLength = Length of the message that was received from the IoT Hub.
 * 
 * Return value = bool - True if all values properly set, otherwise false (mostly due to allocation errors, or unknown setting / property names)
 *****************************************************************************************************************************************************/
bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, int msgLength)
{
    DEBUGMSG_FUNC_IN("(%d, %p, %d)", (int)updateState, message, msgLength);
    updateReportedValues = false;
    JSON_Value *root_value = json_parse_string(message);
    
#ifdef LOGGING
    char szBuffer[MAX_RAW_MSG_BUFFER_SIZE+1];

    for (int i = 0; i < msgLength; i += MAX_RAW_MSG_BUFFER_SIZE) {
        snprintf(szBuffer, min(MAX_RAW_MSG_BUFFER_SIZE+1, msgLength - i + 1), message+i);
        DEBUGMSG_RAW(ZONE_RAWDATA, szBuffer);
    }

    DEBUGMSG_RAW(ZONE_RAWDATA, "\r\n");
#endif

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        DEBUGMSG(ZONE_ERROR, "parsing %p failed", message);
        DEBUGMSG_FUNC_OUT(" = false");
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
                    DEBUGMSG_FUNC_OUT(" = false");
                    return false;
                }
            }

            char szReportedValue[80];    // Potential Risk of overflowing the array
            sprintf(szReportedValue, "reported.%s", deviceTwinEntries[iValues].pszName);
            if (json_object_dothas_value(root_object, szReportedValue)) {
                if (! SetReportedTwinValue(iValues, root_object, szReportedValue)) {
                    DEBUGMSG_FUNC_OUT(" = false");
                    return false;
                }
            } else {    // We have a new desired value that does not yet have a reported value, so assign the desired value to the reported value immediately.
                updateReportedValues = true;
                switch (deviceTwinEntries[iValues].twinDataType) {
                    case VALUE_INT:
                        SetReportedIntValue(iValues, NULL, NULL);
                        break;
                    case VALUE_ULONG:
                        SetReportedUlongValue(iValues, NULL, NULL);
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
                        DEBUGMSG(ZONE_ERROR, "undefined Twin Datatype for reportedTwinValues[%d] = %d", iValues, (int)deviceTwinEntries[iValues].twinDataType);
                        DEBUGMSG_FUNC_OUT(" = false");
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
                    case VALUE_ULONG:
                        SetReportedUlongValue(iValues, NULL, NULL);
                        break;
                    case VALUE_FLOAT:
                        SetReportedFloatValue(iValues, NULL, NULL);
                        break;
                    case VALUE_BOOL:
                        SetReportedBoolValue(iValues, NULL, NULL);
                        break;
                    case VALUE_STRING:
                        if (! SetReportedStringValue(iValues, NULL, NULL)) {
                            DEBUGMSG_FUNC_OUT(" = false");
                            return false;
                        }
                        break;
                    default:
                        DEBUGMSG(ZONE_ERROR, "undefined Twin Datatype for reportedTwinValues[%d] = %d", iValues, (int)deviceTwinEntries[iValues].twinDataType);
                        DEBUGMSG_FUNC_OUT(" = false");
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
                    DEBUGMSG_FUNC_OUT(" = false");
                    return false;
                }

            }
        }
    }
    json_value_free(root_value);
    isInitialized = true;
    DEBUGMSG_FUNC_OUT(" = true");
    return true;
}

bool CreateTelemetryMessage(char *payload, uint64_t upTime, uint64_t sensorReads, uint64_t telemetrySend, bool forceCreate)
{
#ifdef LOGGING
    char szUp[ULONG_64_MAX_DIGITS+1];
    char szSensorReads[ULONG_64_MAX_DIGITS+1];
    char szTelemsSend[ULONG_64_MAX_DIGITS+1];
    Int64ToString(upTime, szUp, ULONG_64_MAX_DIGITS);
    Int64ToString(sensorReads, szSensorReads, ULONG_64_MAX_DIGITS);
    Int64ToString(telemetrySend, szTelemsSend, ULONG_64_MAX_DIGITS);
#endif
    DEBUGMSG_FUNC_IN("(%p, %s, %s, %s, %s)", payload, szUp, szSensorReads, szTelemsSend, ShowBool(forceCreate));

    bool bReadHumidity = reportedTwinValues[IDX_READHUMIDITY].bValue;
    bool bReadPressure = reportedTwinValues[IDX_READPRESSURE].bValue;
    float t = ReadTemperature() + reportedTwinValues[IDX_TEMPERATURECORRECTION].fValue;
    float h = ReadHumidity() + reportedTwinValues[IDX_HUMIDITYCORRECTION].fValue;
    float p = ReadPressure() + reportedTwinValues[IDX_PRESSURECORRECTION].fValue;
    float deltaT = abs(telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue - t);
    float deltaH = abs(telemetryValues[IDX_HUMIDITY_TELEMETRY].fValue - h);
    float deltaP = abs(telemetryValues[IDX_PRESSURE_TELEMETRY].fValue - p);
    bool temperatureAlert = false;

    DEBUGMSG(ZONE_SENSORDATA, "deltaT = %.2f, deltaH = %.2f, deltaP = %.2f", deltaT, deltaH, deltaP);
    DEBUGMSG(ZONE_SENSORDATA, "reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue = %.1f", reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue);
    DEBUGMSG(ZONE_SENSORDATA, "reportedTwinValues[IDX_HUMIDITYACCURACY].fValue = %.1f", reportedTwinValues[IDX_HUMIDITYACCURACY].fValue);
    DEBUGMSG(ZONE_SENSORDATA, "reportedTwinValues[IDX_PRESSUREACCURACY].fValue = %.1f", reportedTwinValues[IDX_PRESSUREACCURACY].fValue);
    DEBUGMSG(ZONE_SENSORDATA, "reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue = %.1f", reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue);

    bool temperatureChanged = (deltaT > reportedTwinValues[IDX_TEMPERATUREACCURACY].fValue);
    bool humidityChanged    = (deltaH > reportedTwinValues[IDX_HUMIDITYACCURACY].fValue);
    bool pressureChanged    = (deltaP > reportedTwinValues[IDX_PRESSUREACCURACY].fValue);

    if (forceCreate) {
        temperatureChanged = humidityChanged = pressureChanged = true;
    }

    DEBUGMSG(ZONE_SENSORDATA, "temperatureChanged = %s, humidityChanged = %s, pressureChanged = %s", ShowBool(temperatureChanged), ShowBool(humidityChanged), ShowBool(pressureChanged));
    DEBUGMSG(ZONE_SENSORDATA, "temperature = %.1f, humidity = %.1f, pressure = %.1f", telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue, telemetryValues[IDX_HUMIDITY_TELEMETRY].fValue, telemetryValues[IDX_PRESSURE_TELEMETRY].fValue);
    DEBUGMSG(ZONE_SENSORDATA, "t = %.1f, h = %.1f, p = %.1f", t, h, p);

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

        telemetryValues[IDX_UPTIME_TELEMETRY].ulValue = upTime;
        json_object_set_number(root_object, telemetryEntries[IDX_UPTIME_TELEMETRY].pszName, telemetryValues[IDX_UPTIME_TELEMETRY].ulValue);

        ShowTelemetryData(telemetryValues[IDX_UPTIME_TELEMETRY].ulValue,
                          sensorReads, telemetrySend,
                          telemetryValues[IDX_TEMPERATURE_TELEMETRY].fValue,
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

    DEBUGMSG_FUNC_OUT(" = %s", ShowBool(temperatureAlert));
    return temperatureAlert;
}


void CreateEventMsg(char *payload, IOTC_EVENT_TYPE eventType)
{
    DEBUGMSG_FUNC_IN("(%p, %d)", payload, (int)eventType);
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
    DEBUGMSG_FUNC_OUT("");
}

bool SendDeviceInfo()
{
    DEBUGMSG_FUNC_IN("()");
    char reportedProperties[REPORTED_PROPERTIES_MAX_LEN];
    bool bSuccess = true;
    updateReportedValues = false;

    reportedProperties[0] = '{';
    reportedProperties[1] = '\0';

    for (int iValues = 0; bSuccess == true && iValues < iExpectedValues; iValues++) {
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
                bSuccess = false;
                break;
        }
    }

    if (bSuccess) {
        int idx = strlen(reportedProperties);
        sprintf(&reportedProperties[idx-1], "%c", '}');

#ifdef LOGGING
        char szBuffer[MAX_RAW_MSG_BUFFER_SIZE+1];

        for (int i = 0; i < idx; i += MAX_RAW_MSG_BUFFER_SIZE) {
            snprintf(szBuffer, min(MAX_RAW_MSG_BUFFER_SIZE+1, idx - i + 1), reportedProperties+i);
            DEBUGMSG_RAW(ZONE_RAWDATA, szBuffer);
        }

        DEBUGMSG_RAW(ZONE_RAWDATA, "\r\n");
#endif

        DEBUGMSG(ZONE_RAWDATA, "Sending reportedProperties to IoTHub");
        bSuccess = DevKitMQTTClient_ReportState(reportedProperties);
    }
    DEBUGMSG_FUNC_OUT(" = %s", ShowBool(bSuccess));
    return bSuccess;
}

/******************************************************************************************************************************************************
 * Collection of methods that each return one individual reported twin value. When the reported twin value is not available, the individual methods
 * either return FALSE, NULL or a negative value, depending on the data type of the return value. 
 *****************************************************************************************************************************************************/
bool InitialDeviceTwinDesiredReceived()
{
    DEBUGMSG(ZONE_VERBOSE, "initialDeviceTwinReportReceived = %s", ShowBool(initialDeviceTwinReportReceived));
    return initialDeviceTwinReportReceived;
}

int MeasurementInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "isInitialized = %s, interval = %d", ShowBool(isInitialized), reportedTwinValues[IDX_MEASUREINTERVAL].iValue * 1000);
    return isInitialized ? reportedTwinValues[IDX_MEASUREINTERVAL].iValue * 1000 : -1;
}

int SendInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "isInitialized = %s, interval = %d", ShowBool(isInitialized), reportedTwinValues[IDX_SENDINTERVAL].iValue * 1000);
    return isInitialized ? reportedTwinValues[IDX_SENDINTERVAL].iValue * 1000 : -1;
}

int SleepInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "isInitialized = %s, interval = %d", ShowBool(isInitialized), reportedTwinValues[IDX_SLEEPTIME].iValue);
    return isInitialized ? reportedTwinValues[IDX_SLEEPTIME].iValue : -1;
}

int MotionInterval()
{
    DEBUGMSG(ZONE_VERBOSE, "isInitialized = %s, interval = %d", ShowBool(isInitialized), reportedTwinValues[IDX_MOTIONDETECTIONINTERVAL].iValue * 1000);
    return isInitialized ? reportedTwinValues[IDX_MOTIONDETECTIONINTERVAL].iValue * 1000 : -1;
}

int WarmingUpTime()
{
    DEBUGMSG(ZONE_VERBOSE, "isInitialized = %s, interval = %d", ShowBool(isInitialized), reportedTwinValues[IDX_WARMINGUPTIME].iValue * 60 * 1000);
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

char *CurrentDevice()
{
    return isInitialized ? reportedTwinValues[IDX_DEVICEMODEL].pszValue : NULL;
}
