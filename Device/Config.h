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

#define DEFAULT_WAKEUP_INTERVAL                 500
#define DEFAULT_MEASURE_INTERVAL                10
#define DEFAULT_SEND_INTERVAL                   300
#define DEFAULT_SLEEP_INTERVAL                  500
#define DEFAULT_WARMING_UP_TIME                 2
#define DEFAULT_MOTION_DETECTION_INTERVAL       60 
#define DEFAULT_TEMPERATURE_ACCURACY            0.2
#define DEFAULT_PRESSURE_ACCURACY               0.2
#define DEFAULT_HUMIDITY_ACCURACY               0.5
#define DEFAULT_MOTION_SENSITIVITY              10
#define DEFAULT_TEMPERATURE_CORRECTION          0.0
#define DEFAULT_HUMIDITY_CORRECTION             0.0
#define DEFAULT_PRESSURE_CORRECTION             0.0
#define DEFAULT_DETECT_MOTION                   false
#define DEFAULT_READ_PRESSURE                   true
#define DEFAULT_READ_HUMIDITY                   true
#define DEFAULT_DEBUGMASK                       0x7000
#define MESSAGE_MAX_LEN                         256
#define REPORTED_PROPERTIES_MAX_LEN             1024
#define DEFAULT_TEMPERATURE_ALERT               40
#define DEFAULT_DISPLAY_TIME                    60

#define DEVICE_ID                               "AZ3166-Proto"
#define DEVICE_LOCATION                         "On the road"
#define DEVICE_FIRMWARE_VERSION                 "2.1.1"

// Telemetry
#define JSON_TEMPERATURE                        "temperature"
#define JSON_HUMIDITY                           "humidity"
#define JSON_PRESSURE                           "pressure"
#define JSON_TEMPERATURE_ALERT                  "temperatureAlert"
#define JSON_MOTION_DETECTED                    "motionDetected"
#define JSON_UPTIME                             "upTime"

#define JSON_MESSAGEID                          "messageId"
#define JSON_DEVICEID                           "device_Id"
#define JSON_DEVICELOC                          "device_location"

#define JSON_EVENT_MOTION                       "motionDetected"

#define SIMULATED_DATA                          false

#define ULONG_64_MAX_DIGITS                     20
#define MAX_RAW_MSG_BUFFER_SIZE                 200

// AZ3166 specific values
#define AZ3166_DISPLAY_SIZE                     128
#define AZ3166_RGB_LED_BRIGHTNESS               32

extern void delay( uint32_t ) ;

#endif