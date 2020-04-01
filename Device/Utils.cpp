#line 1 "Utils.cpp"
#include "arduino.h"
#include "AzureIotHub.h"

#include "DebugZones.h"

/********************************************************************************************************
 * Int64ToString is a helper function that allows easy display / logging of long integers. These long integers
 * are mainly used in this application to find out when timers are expired in the main loop, after which
 * some action needs to be taken.
 * 
 * uint64_t uiValue - The 64 bit value to be converted to a ShowString.
 * char *pszValue - Place holder for the converted string.
 * int iLen - Length of the string place holder. NOTE: Make sure to check the length of the converted
 *            value to avoid overwriting memory.
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

/********************************************************************************************************
 * ShowBool is a helper function that allows easy display / logging of boolean values.
 * A boolean string representation is returned to the caller.
 *******************************************************************************************************/
const char *ShowBool(bool theValue)
{
    return theValue ? "true" : "false";
}

/********************************************************************************************************
 * ShowString is a helper function that allows easy display of strings (including NULL values).
 * The display string is returned to the caller(either containing the original string or "NULL")
 *******************************************************************************************************/
const char *ShowString(char *pszTheValue)
{
    return pszTheValue != NULL ? pszTheValue : "NULL";
}
