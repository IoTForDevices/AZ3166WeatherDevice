#line 1 "Utils.cpp"
#include "arduino.h"
#include "AzureIotHub.h"

#include "DebugZones.h"

/********************************************************************************************************
 * Int64ToString is a helper function that allows easy display / logging of long integers. These long integers
 * are mainly used in this application to find out when timers are expired in the main loop, after which
 * some action needs to be taken.
 *******************************************************************************************************/
void Int64ToString(uint64_t uiValue, char *pszValue, int iLen)
{
    DEBUGMSG_FUNC_IN("(%p, %d)", pszValue, iLen);
    if (uiValue == 0) {
        *pszValue = '\0';
        return;
    }

    const int NUM_DIGITS = (int)floor(log10(uiValue)) + 1;

    DEBUGMSG(ZONE_VERBOSE, "digits = %d", NUM_DIGITS);

    if (NUM_DIGITS < 0 || iLen < NUM_DIGITS) {
        *(pszValue+0) = '-';
        *(pszValue+1) = '\0';
    } else {
        *(pszValue+NUM_DIGITS) = '\0';

        for (size_t i = NUM_DIGITS; i--; uiValue /= 10) {
            *(pszValue+i) = '0' + (uiValue % 10);
        }
    }
    DEBUGMSG_FUNC_OUT("");
}

const char *ShowBool(bool theValue)
{
    return theValue ? "true" : "false";
}

const char *ShowString(char *pszTheValue)
{
    return pszTheValue != NULL ? pszTheValue : "NULL";
}
