#include "Arduino.h"
#include "AzureIotHub.h"
#include "parson.h"
#include "UpdateFirmwareOTA.h"

#define DIAGNOSTIC_INFO_DM_NOT


void BuildResponseString(const char *payLoad, unsigned char **response, int *responseLength)
{
#ifdef DIAGNOSTIC_INFO_DM
    LogInfo("BuildResponseString called with payload: %s", payLoad);
    delay(200);
#endif
    *responseLength = strlen(payLoad);
    *response = (unsigned char *)malloc(*responseLength);
    strncpy((char *)(*response), payLoad, *responseLength);
}

bool HandleReset(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
#ifdef DIAGNOSTIC_INFO_DM
    LogInfo("HandleReset called");
#endif
    BuildResponseString(ok, response, responseLength);
    return true;
}

bool HandleFirmwareUpdate(const char *payload, int payloadLength, unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
#ifdef DIAGNOSTIC_INFO_DM
    LogInfo("HandleFirmwareUpdate called");
    LogInfo("Payload = %s", payload);
#endif

    JSON_Value *root_value;
    root_value = json_parse_string(payload);
    
    bool sendAck = false;

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        LogError("parse %s failed", payload);
        return sendAck;
    }

    JSON_Object *root_object = json_value_get_object(root_value);

    sendAck = true;
    const char *pszFWVersion, *pszFWLocation, *pszFWChecksum;
    int fileSize;

    if (json_object_dothas_value(root_object, "firmwareVersion")) {
        pszFWVersion = json_object_get_string(root_object, "firmwareVersion");
    }
    if (json_object_dothas_value(root_object, "firmwareLocation")) {
        pszFWLocation = json_object_get_string(root_object, "firmwareLocation");
    }
    if (json_object_dothas_value(root_object, "firmwareChecksum")) {
        pszFWChecksum = json_object_dotget_string(root_object, "firmwareChecksum");
    }
    if (json_object_dothas_value(root_object, "firmwareFileSize")) {
        fileSize = (int)json_object_dotget_number(root_object, "firmwareFileSize");
    }

    SetNewFirmwareInfo(pszFWVersion, pszFWLocation, pszFWChecksum, fileSize);
    BuildResponseString(ok, response, responseLength);
    json_value_free(root_value);
    return sendAck;
}

bool HandleMeasureNow(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
#ifdef DIAGNOSTIC_INFO_DM
    LogInfo("HandleMeasureNow called");
#endif
    BuildResponseString(ok, response, responseLength);
    return true;
}

bool HandleUnknownMethod(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"NOK\"}";
#ifdef DIAGNOSTIC_INFO_DM
    LogInfo("HandleUnknownMethod");
#endif
    BuildResponseString(ok, response, responseLength);
    return true;
}
