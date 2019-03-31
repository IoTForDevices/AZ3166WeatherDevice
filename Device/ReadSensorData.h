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

#ifndef READSENSORDATA_H
#define READSENSORDATA_H

void SetupSensors();
double Round(float);
float ReadHumidity();
float ReadTemperature();
float ReadPressure();
bool MotionDetected(int);
bool IsButtonClicked(unsigned char);

#endif // READSENSORDATA_H
