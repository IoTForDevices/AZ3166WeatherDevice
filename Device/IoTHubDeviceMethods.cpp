#line 1 "IoTHubDeviceMethods.cpp"
#include "Arduino.h"
#include "AzureIotHub.h"
#include "parson.h"
#include "UpdateFirmwareOTA.h"
#include "DebugZones.h"
#include "utils.h"


void BuildResponseString(const char *payLoad, unsigned char **response, int *responseLength)
{
    DEBUGMSG_FUNC_IN("(%s, %p, %p)", payLoad, *response, responseLength);
    *responseLength = strlen(payLoad);
    *response = (unsigned char *)malloc(*responseLength);
    strncpy((char *)(*response), payLoad, *responseLength);
    DEBUGMSG(ZONE_HUBMSG, "response = %s, responseLength = %d", *response, *responseLength);
    DEBUGMSG_FUNC_OUT("");
}

bool HandleReset(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
    DEBUGMSG_FUNC_IN("(%p, %p)", *response, responseLength);
    BuildResponseString(ok, response, responseLength);
    DEBUGMSG_FUNC_OUT(" = true");
    return true;
}

bool HandleFirmwareUpdate(const char *payload, int payloadLength, unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
    DEBUGMSG_FUNC_IN("(%s, %d, %p, %p)", payload, payloadLength, *response, responseLength);

    JSON_Value *root_value;
    root_value = json_parse_string(payload);
    
    bool sendAck = false;

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        DEBUGMSG(ZONE_ERROR, "parsing %s failed", payload);
        DEBUGMSG_FUNC_OUT(" = true");

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
    DEBUGMSG_FUNC_OUT(" = %s", ShowBool(sendAck));
    return sendAck;
}

bool HandleResetFWVersion(const char *payload, int payloadLength, unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
    DEBUGMSG_FUNC_IN("(%s, %d, %p, %p)", payload, payloadLength, *response, responseLength);

    JSON_Value *root_value;
    root_value = json_parse_string(payload);
    
    bool sendAck = false;

    if (json_value_get_type(root_value) != JSONObject) {
        if (root_value != NULL) {
            json_value_free(root_value);
        }
        DEBUGMSG(ZONE_ERROR, "parsing %s failed", payload);
        DEBUGMSG_FUNC_OUT(" = false");
        return sendAck;
    }

    JSON_Object *root_object = json_value_get_object(root_value);

    sendAck = true;
    const char *pszFWVersion, *pszFWLocation, *pszFWChecksum;
    int fileSize;

    if (json_object_dothas_value(root_object, "desiredFWVersion")) {
        pszFWVersion = json_object_get_string(root_object, "desiredFWVersion");
    }

    ResetFirmwareInfo(pszFWVersion);
    BuildResponseString(ok, response, responseLength);
    json_value_free(root_value);
    DEBUGMSG_FUNC_OUT(" = %s", ShowBool(sendAck));
    return sendAck;
}

bool HandleMeasureNow(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
    DEBUGMSG_FUNC_IN("(%p, %p)", *response, responseLength);
    BuildResponseString(ok, response, responseLength);
    DEBUGMSG_FUNC_OUT(" = true");
    return true;
}

bool HandleUnknownMethod(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"NOK\"}";
    DEBUGMSG_FUNC_IN("(%p, %p)", *response, responseLength);
    BuildResponseString(ok, response, responseLength);
    DEBUGMSG_FUNC_OUT(" = true");
    return true;
}
