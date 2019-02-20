# AZ3166WeatherDevice
## Weather Station, based on a MXChip IoT DevKit device

This sample connects to an IoT Hub (including IoT Hub as part of an Azure IoT Central Solution)

Telemetry being send is any combination of the following values:

name | data type
-----|----------
temperature | float
humidity | float
pressure | float

The frequency of sending Telemetry Data is configurable. The accuracy of telemetry data is configurable as well.

The following settings are used in the application:

name | data type
-----|----------
measureInterval | int (value in seconds)
sendInterval | int (value in seconds)
warmingUpTime | int (value in minutes)
temperatureAlert | int
temperatureAccuracy | float
pressureAccuracy | float
humidityAccuracy | float
maxDeltaBetweenMeasurements | int
temperatureCorrection | float
pressureCorrection | float
humidityCorrection | float

## Connecting another device to the IoT Central Application

Since GA, IoT Central makes use of DPS to securily connect and provision devices. The new MXChip firmware supports this. However, if you want to make use
of existing code on your MXChip device, you probably need to manually set the connection string to IoT Central. Execute these steps to get a connetion string from your device. The simplest way is to do this from your Azure Cloud Shell:

- mkdir ~/MXChip
- cd ~/MXChip
- npm install dps-keygen
- wget https://github.com/Azure/dps-keygen/blob/master/bin/linux/dps_cstr?raw=true -O dps_cstr
- chmod +x dps_cstr

This is the time to find the connection details of your physical device in IoT Central in order to build a connection string to the device. You need the following information:

- Scope ID
- Device ID
- Primary Key

Now execute the following command inside the Azure Cloud Shell:

- ./dps_cstr [Scope ID] [Device ID] [Primary Key]

## OTA updates

The same mechanism is used that is shown in the Azure IoT Workbench OTA example for the AZ3166. The steps are:

- Provide updated code
- Increase version number in Config.h file
- Compile only and store the .bin file in an Azure blob
- Calculate the checksum and record the file size of the new version of the firmware (workbench functionaly available)
- Use the Update Firmware Command inside Azure IoT Central Weather Station app to trigger an update.
- 



