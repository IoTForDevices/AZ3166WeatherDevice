/*****************************************************************************
 * Functions to read Sensor Data (temperature, humidity & pressure)
 * 
 * NOTE: The read values vary frequently. Right now it is up to the call
 * to deal with the precision of the read values. Currently there is no
 * value calibration. In future that might be desirable.
 * 
 * Copyright (c) IoTForDevices. All rights reserved.
 * Licensed under the MIT license. 
 *****************************************************************************/

#ifndef UPDATEFIRMWAREOTA_H
#define UPDATEFIRMWAREOTA_H

void ReportOTAStatus(const char*, const char*, const char*, const char*, const char*, const char*);
void OTAUpdateFailed(const char*);
void CheckNewFirmware(void);

#endif // UPDATEFIRMWAREOTA_H