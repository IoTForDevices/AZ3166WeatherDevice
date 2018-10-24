#include "Arduino.h"
#include "AzureIoTHub.h"

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
