/*****************************************************************************
 * Functions to read check for new Firmware versions and when found,
 * update the firmware Over The Air.
 * 
 * Copyright (c) IoTForDevices. All rights reserved.
 * Licensed under the MIT license. 
 * 
 * NOTE: ResetFirmwareInfo can only be used with template version 1.0.4
 *       in the IOTC application. Other versions don't expose the command.
 *****************************************************************************/

#ifndef UPDATEFIRMWAREOTA_H
#define UPDATEFIRMWAREOTA_H

void FreeFwInfo();
void ReportOTAStatus(const char*, const char*, const char*, const char*, const char*, const char*);
void OTAUpdateFailed(const char*);
void SetNewFirmwareInfo(const char *pszFWVersion, const char *pszFWLocation, const char *pszFWChecksum, int fileSize);
void ResetFirmwareInfo(const char *pszDesiredFWVersion);
void CheckNewFirmware();
void CheckResetFirmwareInfo();

#endif // UPDATEFIRMWAREOTA_H