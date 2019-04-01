#ifndef IOTHUBMESSAGEHANDLING_H
#define IOTHUBMESSAGEHANDLING_H

typedef enum {
    MOTION_EVENT
} IOTC_EVENT_TYPE;

typedef struct {
    int measureInterval;                // Different Intervals
    int sendInterval;
    int sleepInterval;
    int warmingUpTime;
    int motionDetectionInterval;
    int upTime;                         // Device uptime in seconds
    int mImsec;                         // For internal use only - conversion of all intervals into msec
    int sImsec;
    int wUTmsec;
    int dSmsec;                         // Device sleep time in msec
    int motionInMsec;
    int temperatureAlert;               // High temperature alert value
    float temperatureAccuracy;          // Different sensor reading accuracies
    float pressureAccuracy;
    float humidityAccuracy;
    int maxDeltaBetweenMeasurements;    // Maximum value that two sensor readings can change without considering it to be false readings
    float temperatureCorrection;        // Sensor reading correction factors
    float pressureCorrection;
    float humidityCorrection;
    int motionSensitivity;              // Number to indicate when motion detected between two accelerator readings
    bool enableHumidityReading;         // Flags to allow suppressing sending of Humidity / Pressure sensor readings
    bool enablePressureReading;
    bool enableMotionDetection;
} DEVICE_SETTINGS;

typedef struct {
    char *pszCurrentFwVersion;
    char *pszDeviceModel;
    char *pszLocation;
} DEVICE_PROPERTIES;

bool CreateTelemetryMessage(char *payload, bool forceCreate, DEVICE_SETTINGS *pDeviceSettings);
void CreateEventMsg(char* payLoad, IOTC_EVENT_TYPE eventType);
bool SendDeviceInfo(DEVICE_SETTINGS *pDeviceSettings, DEVICE_PROPERTIES *pDeviceProperties);
bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, DEVICE_SETTINGS *pDesiredDeviceSettings, DEVICE_SETTINGS *pReportedDeviceSettings, DEVICE_PROPERTIES *pReportedDeviceProperties);
bool InitialDeviceTwinDesiredReceived();
bool SendReportedDeviceTwinValues();

#endif // IOTHUBMESSAGEHANDLING_H