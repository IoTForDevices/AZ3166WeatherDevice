/*****************************************************************************
 * Functions to read check for new Firmware versions and when found,
 * update the firmware Over The Air.
 * 
 * Copyright (c) IoTForDevices. All rights reserved.
 * Licensed under the MIT license. 
 *****************************************************************************/

#ifndef UPDATEFIRMWAREOTA_H
#define UPDATEFIRMWAREOTA_H

void FreeFwInfo();
void ReportOTAStatus(const char*, const char*, const char*, const char*, const char*, const char*);
void OTAUpdateFailed(const char*);
void SetNewFirmwareInfo(const char *pszFWVersion, const char *pszFWLocation, const char *pszFWChecksum, int fileSize);
void CheckNewFirmware();

#endif // UPDATEFIRMWAREOTA_H