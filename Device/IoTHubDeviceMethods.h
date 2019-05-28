/**************************************************************************************************
 * Module Name: IoTHubDeviceMethods.h
 * Version:     2.0.9
 * Description: Definition of all C2D methods that are available for this application.
 * Copyright (c) IoTForDevices. All rights reserved.
 * Licensed under the MIT license. 
 ***********************************************************************************************/
#ifndef IOTHUBDEVICEMETHODS_H
#define IOTHUBDEVICEMETHODS_H

bool HandleReset(unsigned char **response, int *responseLength);
bool HandleMeasureNow(unsigned char **response, int *responseLength);
bool HandleFirmwareUpdate(const char *payload, int payloadLength, unsigned char **response, int *responseLength);
bool HandleResetFWVersion(const char *payload, int payloadLength, unsigned char **response, int *responseLength);
void HandleUnknownMethod(unsigned char **response, int *responseLength);

#endif  // IOTHUBDEVICEMETHODS_H