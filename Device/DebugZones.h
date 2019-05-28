/**************************************************************************************************
 * Module Name: DebugZones.h
 * Version:     2.0.9
 * Description: Definition of a number of macro's that help debugging the solution.
 *              These macro's allow more or less debug information to be displayed, allowing to 
 *              zoom in into problem area's. Debugging information is displayed in the VS Code
 *              terminal window (or any other remote terminal application), being send from the
 *              target over USB (RS323 over USB). The global variable dpCurSettings determines
 *              which data is to be displayed. This value can be set in code, but can also be
 *              set dynamically through the digital twin.
 *              NOTE: When enabled (meaning the environment varialbe LOGGING is defined), the
 *                    application grows significantly because all debug info is not part of the
 *                    binary. When LOGGING is not defined, debug info is not part of the binary
 *                    and the digital twin value is being ignored. When building the 
 *                    application for OTA, make sure that LOGGING is not enabled.
 * Copyright (c) IoTForDevices. All rights reserved.
 * Licensed under the MIT license. 
 ***********************************************************************************************/
#ifndef DEBUG_ZONES_H
#define DEBUG_ZONES_H

#define LOGGING_NOT

#ifdef LOGGING

typedef struct _DBGPARAM {
    char	lpszName[32];           // Name of module
    char    rglpszZones[16][32];    // Names of zones for first 16 bits
    ulong   ulZoneMask;             // Current zone Mask
} DBGPARAM, *LPDBGPARAM;

extern "C" DBGPARAM dpCurSettings;

#define ZONEMASK(n) (0x00000001<<(n))
#define DEBUGZONE(n)  (dpCurSettings.ulZoneMask & (0x00000001<<(n)))

// Definitions for our debug zones                               
#define ZONE0_TEXT            "Init"
#define ZONE1_TEXT            "Function"
#define ZONE2_TEXT            "Memory"
#define ZONE3_TEXT            "Twin Parsing"
#define ZONE4_TEXT            "IoT Hub Msg Handling"
#define ZONE5_TEXT            "Raw Data"
#define ZONE6_TEXT            "Sensor Data"
#define ZONE7_TEXT            "Main Loop"
#define ZONE8_TEXT            "Device Methods"
#define ZONE9_TEXT            "Motion Detection"
#define ZONE10_TEXT           "Firmware OTA Update"
#define ZONE11_TEXT           ""
#define ZONE12_TEXT           "Verbose"
#define ZONE13_TEXT           "Info"
#define ZONE14_TEXT           "Warning"
#define ZONE15_TEXT           "Error"

// These macros can be used as condition flags for LOGMSG
#define ZONE_INIT             DEBUGZONE(0)
#define ZONE_FUNCTION         DEBUGZONE(1)
#define ZONE_MEMORY           DEBUGZONE(2)
#define ZONE_TWINPARSING      DEBUGZONE(3)
#define ZONE_HUBMSG           DEBUGZONE(4)
#define ZONE_RAWDATA          DEBUGZONE(5)
#define ZONE_SENSORDATA       DEBUGZONE(6)
#define ZONE_MAINLOOP         DEBUGZONE(7)
#define ZONE_DEVMETHODS       DEBUGZONE(8)
#define ZONE_MOTIONDETECT     DEBUGZONE(9)
#define ZONE_FWOTAUPD         DEBUGZONE(10)
#define ZONE_11               DEBUGZONE(11)
#define ZONE_VERBOSE          DEBUGZONE(12)
#define ZONE_INFO             DEBUGZONE(13)
#define ZONE_WARNING          DEBUGZONE(14)
#define ZONE_ERROR            DEBUGZONE(15)

// These macros can be used to indicate which zones to enable by default (see RETAILZONES and DEBUGZONES below)
#define ZONEMASK_INIT         ZONEMASK(0)
#define ZONEMASK_FUNCTION     ZONEMASK(1)
#define ZONEMASK_MEMORY       ZONEMASK(2)
#define ZONEMASK_TWINPARSING  ZONEMASK(3)
#define ZONEMASK_HUBMSG       ZONEMASK(4)
#define ZONEMASK_RAWDATA      ZONEMASK(5)
#define ZONEMASK_SENSORDATA   ZONEMASK(6)
#define ZONEMASK_MAINLOOP     ZONEMASK(7)
#define ZONEMASK_DEVMETHODS   ZONEMASK(8)
#define ZONEMASK_MOTIONDETECT ZONEMASK(9)
#define ZONEMASK_FWOTAUPD     ZONEMASK(10)
#define ZONEMASK_11           ZONEMASK(11)
#define ZONEMASK_VERBOSE      ZONEMASK(12)
#define ZONEMASK_INFO         ZONEMASK(13)
#define ZONEMASK_WARNING      ZONEMASK(14)
#define ZONEMASK_ERROR        ZONEMASK(15)

#define DEBUGZONES      ZONEMASK_INIT | ZONEMASK_TWINPARSING | ZONEMASK_RAWDATA | ZONEMASK_ERROR | ZONEMASK_WARNING
#endif

#ifdef LOGGING
#define DEBUGMSG(cond, format, ...) \
{ \
    if (cond) { \
        LogInfo(format, __VA_ARGS__); \
        delay(200); \
    } \
}
#else
#define DEBUGMSG(cond, format, ...) ;
#endif

#endif