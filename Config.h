/*****************************************************************************
 * Physical device information for board and sensors
 * Modify these settings for different behavior and keep unique / device
 * 
 * NOTE: There is a tight relation between measurements and settings inside
 * the device app and their corresponding definitions in the IoTCentral app.
 * 
 * Copyright (c) IoTForDevices. All rights reserved.
 * Licensed under the MIT license. 
 *****************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_WAKEUP_INTERVAL 500
#define DEFAULT_MEASURE_INTERVAL_MSEC 10000
#define DEFAULT_SEND_INTERVAL_MSEC 300000
#define DEFAULT_WARMING_UP_TIME_MSEC 120000
#define DEFAULT_MEASURE_INTERVAL 10
#define DEFAULT_SEND_INTERVAL 300
#define DEFAULT_WARMING_UP_TIME 2

#define DEFAULT_TEMPERATURE_ACCURACY 0.2
#define DEFAULT_PRESSURE_ACCURACY    0.2
#define DEFAULT_HUMIDITY_ACCURACY    0.5
#define DEFAULT_MAX_DELTA_BETWEEN_MEASUREMENTS 5

#define MESSAGE_MAX_LEN 256
#define DEFAULT_TEMPERATURE_ALERT 40

#define DEVICE_ID "AZ3166-WS01"
#define DEVICE_LOCATION "Home - Living Room"
#define DEVICE_FIRMWARE_VERSION "1.7.0"

// Telemetry
#define JSON_TEMPERATURE       "temperature"
#define JSON_HUMIDITY          "humidity"
#define JSON_PRESSURE          "pressure"
#define JSON_TEMPERATURE_ALERT "temperatureAlert"
#define JSON_UPTIME            "upTime"

#define JSON_MESSAGEID         "messageId"
#define JSON_DEVICEID          "deviceId"
#define JSON_DEVICELOC         "device_location"

#define JSON_EVENT_WARNING     "buttonAPressed"
#define JSON_EVENT_ERROR       "buttonBPressed"

#define SIMULATED_DATA false

#endif