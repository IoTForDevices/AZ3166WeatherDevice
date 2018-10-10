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




