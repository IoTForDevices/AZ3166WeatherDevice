#include "Arduino.h"
#include "AzureIoTHub.h"

void BuildResponseString(const char *payLoad, unsigned char **response, int *responseLength)
{
    LogInfo("BuildResponseString called with payload: %s", payLoad);
    *responseLength = strlen(payLoad);
    *response = (unsigned char *)malloc(*responseLength);
    strncpy((char *)(*response), payLoad, *responseLength);
}

bool HandleReset(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"OK\"}";
    LogInfo("HandleReset called");
    BuildResponseString(ok, response, responseLength);
    return true;
}

bool HandleUnknownMethod(unsigned char **response, int *responseLength)
{
    const char *ok = "{\"result\":\"NOK\"}";
    LogInfo("HandleUnknownMethod");
    BuildResponseString(ok, response, responseLength);
    return true;
}
